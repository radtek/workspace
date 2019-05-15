// =====================================================================================
//	Copyright (C) 2019 by Jiaxing Shao.All rights reserved.
//	文 件 名:  websocket_server.cpp
//	作    者:  Jiaxing Shao, 13315567369@163.com
//	版 本 号:  1.0
//	创建时间:  2019年05月06日 18时14分37秒
//	Compiler:  g++
//	描    述:  
// =====================================================================================

#include "websocket_server.h"
#include <jsoncpp/json/json.h>
#include "logfile.h"
#include "rtsp_task.h"
#include "rtsp_struct.h"
#include "rtsp_client.h"
#include "rtsp_server.h"
#include "rtsp_protocol.h"
#include "threadpool.h"
#include "http_handle.h"

extern LOG_QUEUE *log_queue;
extern tcp_server_info *g_rtsp_serv;

WebSocketServer::WebSocketServer()
{
	m_server.init_asio();
	m_server.set_reuse_addr(true);
	m_server.set_open_handler(bind(&WebSocketServer::on_open, this, ::_1));
	m_server.set_close_handler(bind(&WebSocketServer::on_close, this, ::_1));
	m_server.set_message_handler(bind(&WebSocketServer::on_message, this, ::_1, ::_2));
}

WebSocketServer::~WebSocketServer()
{
}

void WebSocketServer::on_open(connection_hdl hdl)
{
	{
		lock_guard<mutex> guard(m_action_lock);
		m_actions.push(action(SUBSCRIBE, hdl));
	}
	m_action_cond.notify_one();
}

void WebSocketServer::on_close(connection_hdl hdl)
{
	{
		lock_guard<mutex> guard(m_action_lock);
		m_actions.push(action(UNSUBSCRIBE, hdl));
	}
	m_action_cond.notify_one();
}

void WebSocketServer::on_message(connection_hdl hdl, message_ptr msg)
{
	{
		lock_guard<mutex> guard(m_action_lock);
		m_actions.push(action(MESSAGE, hdl, msg));
	}
	m_action_cond.notify_one();
}

void WebSocketServer::start(int port)
{
	thread t(process_message, this);
	m_server.listen(port);
	m_server.start_accept();
	try{
		m_server.run();
	} catch(const std::exception &e) {
		std::cout << e.what() << endl;
	}
}

bool WebSocketServer::is_subscribe(int deviceid)
{
	if(m_mapSubscribe.find(deviceid) != m_mapSubscribe.end())
	{
		return true;
	}
	return false;
}

void WebSocketServer::send_video_stream(unsigned int deviceid, char *pData, int nSize)
{
	lock_guard<mutex> guard(m_mapLocks[deviceid]);
	map<unsigned int, conn_list>::iterator iter = m_mapSubscribe.find(deviceid);
	if(iter != m_mapSubscribe.end())
	{
		conn_list::iterator it = iter->second.begin();
		for(; it != iter->second.end(); )
		{
			try {
				m_server.send((*it), pData, nSize, websocketpp::frame::opcode::binary);
				it++;
			} catch (const std::exception &e) {
				log_debug("websocket send_video_stream error, %d.", deviceid);
				it = iter->second.erase(it);
			}
		}
		
		if(iter->second.size() == 0)
		{
			m_mapSubscribe.erase(iter);
		}
	}
}

void *WebSocketServer::process_message(void *arg)
{
	WebSocketServer *pthis = (WebSocketServer*)arg;
	while(true) 
	{
		unique_lock<mutex> lock(pthis->m_action_lock);
		while(pthis->m_actions.empty()) 
		{
			pthis->m_action_cond.wait(lock);
		}
		action act = pthis->m_actions.front();
		pthis->m_actions.pop();
		lock.unlock();
		log_debug("进入%d", act.type);

		if(act.type == SUBSCRIBE)
		{
			lock_guard<mutex> guard(pthis->m_connection_lock);
			pthis->m_connections.insert(act.hdl);
			log_debug("新的websocket连接, Count: %d", pthis->m_connections.size());
		}
		else if(act.type == UNSUBSCRIBE)
		{
			lock_guard<mutex> guard(pthis->m_connection_lock);
			pthis->m_connections.erase(act.hdl);
			log_debug("断开websocket连接, Count: %d", pthis->m_connections.size());

			map<unsigned int, conn_list>::iterator iter = pthis->m_mapSubscribe.begin();
			for(; iter != pthis->m_mapSubscribe.end(); )
			{
				lock_guard<mutex> guard(pthis->m_mapLocks[iter->first]);
				if(iter->second.find(act.hdl) != iter->second.end())
				{
					iter->second.erase(act.hdl);
				}

				if(iter->second.size() == 0)
				{
					iter = pthis->m_mapSubscribe.erase(iter);
				}
				else
				{
					iter++;
				}
			}
			log_debug("断开websocket连接, Over");
		}
		else if(act.type == MESSAGE)
		{
			string msg = act.msg->get_payload();
			log_debug("websocket 接收到数据 %s", msg.c_str());

			Json::Reader reader;
			Json::Value root;
			if(!reader.parse(msg.c_str(), root))
			{
				log_debug("websocket 错误的消息类型, %s", msg.c_str());
				log_info(log_queue, "websocket 错误的消息类型, %s", msg.c_str());
			}

			int deviceid = root["deviceid"].asInt();
			t_device_video_play *player = video_task_get(deviceid);
			if(player == NULL)
			{
				log_debug("websocket 不存在的设备ID, %d", deviceid);
				log_info(log_queue, "websocket 不存在的设备ID, %d", deviceid);
			}
			else
			{
				string ret;
				pthread_mutex_lock(&player->lock);
				if(player->stop)
				{
					t_threadpool_task *task = create_threadpool_task();
					task->callback = &get_video_stream;
					task->arg = (void *)player;
					threadpool_add_task(task);
				}
				pthread_mutex_unlock(&player->lock);
				log_info(log_queue, "websocket 请求视频流, 设备ID %d", deviceid);
				lock_guard<mutex> guard(pthis->m_mapLocks[deviceid]);
				pthis->m_mapSubscribe[deviceid].insert(act.hdl);

					/*
					player->sockfd = connect_server(player->device_info->ipaddr, player->device_info->rtspport);
					if(player->sockfd == -1)
					{
						pthread_mutex_unlock(&player->lock);
						ret = "can't connect to device.";
						log_debug("连接设备 %d 失败, ip[%s], 请检查设备网络状况.");
						log_info(log_queue, "连接设备 %d 失败, ip[%s], 请检查设备网络状况.", deviceid, player->device_info->ipaddr);
						continue;
					}
					if(!rtsp_request(player))
					{
						pthread_mutex_unlock(&player->lock);
						ret = "rtsp protocol wrong";
						log_debug("连接设备 %d 失败, ip[%s], 请检查设备网络状况.");
						log_info(log_queue, "与设备 %d rtsp对接失败,ip[%s]", deviceid, player->device_info->ipaddr);
						continue;
					}
					// RTSP服务提供标志
					player->stop = false;
					start_byte_array(player->rtp_array);
					g_rtsp_serv->device[player->serv_pos]->stop = false;
					g_rtsp_serv->device[player->serv_pos]->time_count = 0;
					log_info(log_queue, "连接到设备 %d 成功,ip[%s],开始接收视频流", deviceid, player->device_info->ipaddr);
					pthread_create(&player->pid[0], NULL, rtsp_worker_start, (void*)&player->device_info->deviceid);
					pthread_create(&player->pid[1], NULL, byte_array_process_start, (void*)&player->device_info->deviceid);
					*/
			}
		}
		else
		{
			// undefined
		}
	}
}

