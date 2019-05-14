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

extern LOG_QUEUE *log_queue;

WebSocketServer::WebSocketServer()
{
	m_server.init_asio();
	m_server.set_open_handler(bind(&WebSocketServer::on_open, this, ::_1));
	m_server.set_close_handler(bind(&WebSocketServer::on_close, this, ::_1));
	m_server.set_message_handler(bind(&WebSocketServer::on_message, this, ::_1, ::_2));
}

WebSocketServer::~WebSocketServer()
{
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

void WebSocketServer::send_video_stream(unsigned int deviceid, char *pData, int nSize)
{
	if(m_mapSubscribe.find(deviceid) != m_mapSubscribe.end())
	{
		lock_guard<mutex> guard(m_mapLocks[deviceid]);
		conn_list::iterator iter = m_mapSubscribe[deviceid].begin();
		for(; iter != m_mapSubscribe[deviceid].end(); iter++)
		{
			m_server.send((*iter), pData, nSize, websocketpp::frame::opcode::binary);
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
		}
		else if(act.type == UNSUBSCRIBE)
		{
			lock_guard<mutex> guard(pthis->m_connection_lock);
			pthis->m_connections.erase(act.hdl);

			/*
			map<unsigned int, conn_list>::iterator iter = pthis->m_mapSubscribe.begin();
			for(; iter != pthis->m_mapSubscribe.end(); )
			{
				{
					map<unsigned int, mutex>::iterator lockit = pthis->m_mapLocks.find(iter->first);
					if(lockit == pthis->m_mapLocks.end())
					{
						pthis->m_mapLocks[iter->first];
						lockit = pthis->m_mapLocks.find(iter->first);
					}
					lock_guard<mutex> guard(lockit->second);
					if(iter->second.find(act.hdl) != iter->second.end())
					{
						iter->second.erase(act.hdl);
					}
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
			*/
		}
		else if(act.type == MESSAGE)
		{
			string msg = act.msg->get_payload();
			Json::Reader reader;
			Json::Value root;
			if(!reader.parse(msg.c_str(), root))
			{
				log_debug("websocket 错误的消息类型, %s", msg.c_str());
				log_info(log_queue, "websocket 错误的消息类型, %s", msg.c_str());
				lock_guard<mutex> guard(pthis->m_connection_lock);
				pthis->m_connections.erase(act.hdl);
			}

			int deviceid = root["deviceid"].asInt();
			if(video_task_get(deviceid) == NULL)
			{
				log_debug("websocket 不存在的设备ID, %d", deviceid);
				log_info(log_queue, "websocket 不存在的设备ID, %d", deviceid);
				lock_guard<mutex> guard(pthis->m_connection_lock);
				pthis->m_connections.erase(act.hdl);
			}
			else
			{
				log_debug("websocket 请求视频流, 设备ID %d", deviceid);
				log_info(log_queue, "websocket 请求视频流, 设备ID %d", deviceid);
				map<unsigned int, mutex>::iterator lockit = pthis->m_mapLocks.find(deviceid);
				if(lockit == pthis->m_mapLocks.end())
				{
					pthis->m_mapLocks[deviceid];
					lockit = pthis->m_mapLocks.find(deviceid);
				}
				lock_guard<mutex> guard(lockit->second);
				pthis->m_mapSubscribe[deviceid].insert(act.hdl);
			}
		}
		else
		{
			// undefined
		}
	}
}

