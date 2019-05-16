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

	/*
	lock_guard<mutex> guard(m_connection_lock);
	m_connections.insert(hdl);
	log_debug("新的websocket连接, Count: %d", m_connections.size());
	*/
}

void WebSocketServer::on_close(connection_hdl hdl)
{
	{
		lock_guard<mutex> guard(m_action_lock);
		m_actions.push(action(UNSUBSCRIBE, hdl));
	}
	m_action_cond.notify_one();

	/*
	lock_guard<mutex> guard(m_connection_lock);
	m_connections.erase(hdl);
	log_debug("断开websocket连接, Count: %d", m_connections.size());
	*/
}

void WebSocketServer::on_message(connection_hdl hdl, message_ptr msg)
{
	{
		lock_guard<mutex> guard(m_action_lock);
		m_actions.push(action(MESSAGE, hdl, msg));
	}
	m_action_cond.notify_one();

	/*
	string data = msg->get_payload();
	log_debug("websocket 接收到数据 %s", data.c_str());

	Json::Reader reader;
	Json::Value root;
	if(!reader.parse(data.c_str(), root))
	{
		log_debug("websocket 错误的消息类型, %s", data.c_str());
		log_info(log_queue, "websocket 错误的消息类型, %s", data.c_str());
		return;
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
		lock_guard<mutex> guard(m_mapLocks[deviceid]);
		m_mapSubscribe[deviceid].insert(hdl);
	}
	log_debug("websocket 接收到数据处理完毕");
	*/
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
	if(m_mapSubscribe[deviceid].size() != 0)
	{
		return true;
	}
	return false;
}

void WebSocketServer::send_video_stream(unsigned int deviceid, char *pData, int nSize)
{
	lock_guard<mutex> guard(m_mapLocks[deviceid]);
	if(m_mapSubscribe[deviceid].size() != 0)
	{
		conn_list::iterator iter = m_mapSubscribe[deviceid].begin();
		for(; iter != m_mapSubscribe[deviceid].end();)
		{
			try {
				m_server.send((*iter), pData, nSize, websocketpp::frame::opcode::binary);
				iter++;
			} catch (const std::exception &e) {
				log_debug("websocket send_video_stream error, %d.", deviceid);
				iter = m_mapSubscribe[deviceid].erase(iter);
			}
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
		}
		else if(act.type == MESSAGE)
		{
			string data = act.msg->get_payload();
			log_debug("websocket 接收到数据 %s", data.c_str());

			Json::Reader reader;
			Json::Value root;
			if(!reader.parse(data.c_str(), root))
			{
				log_debug("websocket 错误的消息类型, %s", data.c_str());
				log_info(log_queue, "websocket 错误的消息类型, %s", data.c_str());
				continue;
			}

			int deviceid = root["deviceid"].asInt();
			t_device_video_play *player = video_task_get(deviceid);
			if(player == NULL)
			{
				log_debug("websocket 不存在的设备ID, %d", deviceid);
				log_info(log_queue, "websocket 不存在的设备ID, %d", deviceid);
				continue;
			}

			if(player->stop)
			{
				log_debug("websocket添加播放任务, %d", deviceid);
				t_threadpool_task *task = create_threadpool_task();
				task->callback = &get_video_stream;
				task->arg = (void *)player;
				threadpool_add_task(task, deviceid);
			}

			log_info(log_queue, "websocket 请求视频流, 设备ID %d", deviceid);
			lock_guard<mutex> guard(pthis->m_mapLocks[deviceid]);
			pthis->m_mapSubscribe[deviceid].insert(act.hdl);
		}
		else
		{
			// undefined
		}
	}
}

