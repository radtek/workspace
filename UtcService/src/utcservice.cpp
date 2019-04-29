// =====================================================================================
//	Copyright (C) 2018 by Jiaxing Shao.All rights reserved.
//	文 件 名:  utcservice.cpp
//	作    者:  Jiaxing Shao, 13315567369@163.com
//	版 本 号:  1.0
//	创建时间:  2018年08月13日 18时06分07秒
//	Compiler:  g++
//	描    述:  
// =====================================================================================

#include "utcservice.h"

UtcService::UtcService()
{
	for(int i = 0; i < 2; i++)
	{
		memset(m_subscribeBuf[i], 0, BUF_LEN);
	}
}

UtcService::~UtcService()
{
	if(m_clntCcuConn != NULL)
	{
		delete [] m_clntCcuConn;
		m_clntCcuConn = NULL;
	}
}

void *UtcService::thread_sendXml(void *arg)
{
	UtcService *pthis = (UtcService*)arg;

	while(true)
	{
		pthread_mutex_lock(&g_mutexWaitSendXml);
		while(g_listWaitSendXml.size() == 0)
		{
			pthread_cond_wait(&g_condWaitSendXml, &g_mutexWaitSendXml);
		}
		string strXml = g_listWaitSendXml.front();
		g_listWaitSendXml.pop_front();
		pthread_mutex_unlock(&g_mutexWaitSendXml);

		int n = pthis->m_msgServConn.send_msg(strXml.c_str(), strXml.length());
		if(n < 0)
		{
			g_logs->WriteLog("发送XML消息失败，与消息服务器断开连接，等待重连");
			while(!pthis->m_msgServConn.getConnStat())
			{
				int ret = pthis->m_msgServConn.connect_serv();
				if(ret <= 0)
				{
					g_logs->WriteLog("thread_senXml 连接消息服务器失败，等待重连，错误码:%d", ret);
					sleep(RECONN_TIME);
				}
				else
				{
					pthis->m_msgServConn.send_msg(pthis->m_subscribeBuf[0], pthis->m_subscribeLen[0]);
					pthis->m_msgServConn.send_msg(pthis->m_subscribeBuf[1], pthis->m_subscribeLen[1]);
					g_logs->WriteLog("消息服务器连接成功");
					break;
				}
			} // while(!getConnStat())
		}
		else
		{
			g_logs->WriteLog("发送数据到消息服务器成功");
		}
	}
}

void *UtcService::thread_recvXml(void *arg)
{
	UtcService *pthis = (UtcService*)arg;
	
	char buffer[BUF_LEN * 4];
	int length = 0;
	while(true)
	{
		memset(buffer, 0, BUF_LEN * 4);
		length = pthis->m_msgServConn.recv_msg(buffer, BUF_LEN * 4);
		if(length > 0)
		{
			string strXml = buffer;
			pthread_mutex_lock(&g_mutexWaitDecodeXml);
			g_listWaitDecodeXml.push_back(strXml);
			pthread_mutex_unlock(&g_mutexWaitDecodeXml);
		}
		else
		{
			g_logs->WriteLog("与消息服务器断开连接，等待重连 recv = %d", length);
			while(!pthis->m_msgServConn.getConnStat())
			{
				int ret = pthis->m_msgServConn.connect_serv();
				if(ret <= 0)
				{
					g_logs->WriteLog("thread_recvXml 重新连接消息服务器失败，等待重连，错误码:%d", ret);
					sleep(RECONN_TIME);
				}
				else
				{
					pthis->m_msgServConn.send_msg(pthis->m_subscribeBuf[0], pthis->m_subscribeLen[0]);
					pthis->m_msgServConn.send_msg(pthis->m_subscribeBuf[1], pthis->m_subscribeLen[1]);
					g_logs->WriteLog("消息服务器连接成功");
					break;
				}
			}
		}
	}
}

/* * 
 * 统一的ccu数据发送线程
 * */
void *UtcService::thread_msg_send(void *arg)
{
	pthread_t tid = pthread_self();
	pthread_detach(tid);
	UtcService *pthis = (UtcService*)arg;
	while(true)
	{
		pthread_mutex_lock(&g_mutexWaitSendMsg);
		while(g_listWaitSendMsg.size() == 0)
		{
			// 等待信号
			pthread_cond_wait(&g_condWaitSendMsg, &g_mutexWaitSendMsg);
		}
		T_Message *msg = g_listWaitSendMsg.front();
		g_listWaitSendMsg.pop_front();
		pthread_mutex_unlock(&g_mutexWaitSendMsg);
		for(int i = 0; i < pthis->m_clntCcuCount; i++)
		{
			if((*(pthis->m_clntCcuConn + i))->getConnStat())
			{
				if(msg->crossId == 0)
				{
					string str = ParseMessageHex(msg->buffer, msg->buflen + BUF_HEAD_LEN);
					// crossId 为0,则群发消息
					if((*(pthis->m_clntCcuConn + i))->send_msg(msg->buffer, msg->buflen + BUF_HEAD_LEN) > 0)
					{
						g_logs->WriteLog("发送到CCU %s 成功: %s", (*(pthis->m_clntCcuConn + i))->m_szIpAddr, str.c_str());
					}
					else
					{
						g_logs->WriteLog("发送到CCU %s 失败:%s", (*(pthis->m_clntCcuConn + i))->m_szIpAddr, str.c_str());
					}
					continue;
				}

				if(g_mapCrossInfo[msg->crossId]->ccuip_inetAddr == (*(pthis->m_clntCcuConn + i))->getInetAddr())
				{
					string str = ParseMessageHex(msg->buffer, msg->buflen + BUF_HEAD_LEN);
					if((*(pthis->m_clntCcuConn + i))->send_msg(msg->buffer, msg->buflen + BUF_HEAD_LEN) > 0)
					{
						g_logs->WriteLog("发送到CCU %s 成功: %s", (*(pthis->m_clntCcuConn + i))->m_szIpAddr, str.c_str());
					}
					else
					{
						g_logs->WriteLog("发送到CCU %s 失败:%s", (*(pthis->m_clntCcuConn + i))->m_szIpAddr, str.c_str());
					}
					break;
				}
			}
		}
		if(msg != NULL)
		{
			delete msg;
			msg = NULL;
		}
	}
}

/* * 
 * 每个ccu一个单独的数据接收线程
 * */
void *UtcService::thread_msg_recv(void *arg)
{
	pthread_t tid = pthread_self();
	pthread_detach(tid);

	TcpClient *pconn = (TcpClient*)arg;
	char buffer[BUF_LEN];
	int length = 0;
	while(true)
	{
		if(pconn->getConnStat())
		{
			length = pconn->recv_msg(buffer, BUF_HEAD_LEN);
			if(length <= 0)
			{
				goto ReConn;
			}
			T_Message *msg = new T_Message;
			memset(msg, 0, sizeof(T_Message));
			msg->buflen = *((unsigned short*)buffer);
			msg->crossId = *((unsigned int*)(buffer+2));
			msg->type = buffer[6];
			// 阻塞接收
			length = pconn->recv_msg(msg->buffer, msg->buflen);
			if(length <= 0)
			{
				goto ReConn;
				g_logs->WriteWarn("与CCU连接异常，尝试重新连接");
			}

			pthread_mutex_lock(&g_mapRecvMessageLock[pconn->getInetAddr()]);
			g_mapRecvMessage[pconn->getInetAddr()].push_back(msg);
			pthread_mutex_unlock(&g_mapRecvMessageLock[pconn->getInetAddr()]);
		}
		else
		{
ReConn:
			while(!pconn->getConnStat())
			{
				int ret = pconn->connect_serv();
				if(ret <= 0)
				{
					g_logs->WriteWarn("连接CCU服务失败, ip: %s, port: %d, errcode:%d", pconn->m_szIpAddr, pconn->m_nPort, ret);
					sleep(RECONN_TIME);
				}
				else
				{
					g_logs->WriteWarn("连接CCU服务成功, ip: %s, port: %d, socket fd:%d", pconn->m_szIpAddr, pconn->m_nPort, ret);
					break;
				}
			}
		}
	}
}

void *UtcService::thread_dispatch(void *arg)
{
	UtcService *pthis = (UtcService*)arg;
	map<unsigned int, list<T_Message*> >::iterator mapIter;

	time_t timer = time(NULL);
	while(true)
	{
		// 每30秒，向CCU发送一次心跳包
		if(time(NULL) - timer >= 300)
		{
			timer = time(NULL);
			T_Message *msg = new T_Message;
			memset(msg, 0, sizeof(T_Message));
			msg->crossId = 0;
			*((unsigned int*)(msg->buffer + 2)) = msg->crossId;
			msg->buffer[7] = 0x00;
			msg->buflen = 1;
			*(unsigned short*)msg->buffer = msg->buflen;

			pthread_mutex_lock(&g_mutexWaitSendMsg);
			g_listWaitSendMsg.push_back(msg);
			pthread_mutex_unlock(&g_mutexWaitSendMsg);
			pthread_cond_signal(&g_condWaitSendMsg);
		}

		// 接收到消息服务器数据后，添加任务到线程池
		if(g_listWaitDecodeXml.size() != 0)
		{
			pthread_mutex_lock(&g_mutexWaitDecodeXml);
			if(g_listWaitDecodeXml.size() != 0)
			{
				// 将任务加入到线程池
				string str(g_listWaitDecodeXml.front());
				g_listWaitDecodeXml.pop_front();
				pthread_mutex_unlock(&g_mutexWaitDecodeXml);
				T_Task *task = new T_Task;
				task->type = emDecode;
				task->strXml = str;
				task->msg = NULL;
				g_logs->WriteLog("接收到消息服务器XML消息 %s", str.c_str());
				pthis->m_threadpool.AddTask(task);
			}
			else
			{
				pthread_mutex_unlock(&g_mutexWaitDecodeXml);
			}
		}

		for(mapIter = g_mapRecvMessage.begin(); mapIter != g_mapRecvMessage.end(); mapIter++)
		{
			if(mapIter->second.size() != 0)
			{
				pthread_mutex_lock(&g_mapRecvMessageLock[mapIter->first]);
				if(mapIter->second.size() != 0)
				{
					T_Message *msg = mapIter->second.front();
					mapIter->second.pop_front();
					pthread_mutex_unlock(&g_mapRecvMessageLock[mapIter->first]);
					T_Task *task = new T_Task;
					task->type = emEncode;
					task->msg = msg;
					string str = ParseMessageHex(msg->buffer, msg->buflen);
					g_logs->WriteLog("接收到CCU服务器发送的数据[%d] %s", msg->crossId, str.c_str());
					pthis->m_threadpool.AddTask(task);
				}
				else
				{
					pthread_mutex_unlock(&g_mapRecvMessageLock[mapIter->first]);
				}
			}
		}
		sleep(0.01);
	}	// while(true)
}

void UtcService::Start()
{
	char ipaddr[IP_SIZE] = { 0 };
	char port[PORT_SIZE] = { 0 };
	char transPort[PORT_SIZE] = { 0 };
	GetConfigureString("MessageServIpAddr", ipaddr, IP_SIZE, "127.0.0.1", CONFFILE);
	GetConfigureString("MessageServPort", port, PORT_SIZE, "10086", CONFFILE);
	GetConfigureString("TransServPort", transPort, PORT_SIZE, "21000", CONFFILE);

	m_otlDatabase.OtlInfoInit();
	if(m_otlDatabase.GetCcuInfo(g_vecCcuInfo))
	{
		g_logs->WriteLog("CCU服务信息获取成功");
	}
	if(m_otlDatabase.GetCrossInfo(g_mapCrossInfo))
	{
		g_logs->WriteLog("路口信息获取成功");
	}

	// 连接所有CCU服务器
	m_clntCcuCount = g_vecCcuInfo.size();
	g_logs->WriteLog("共获取到%d个CCU服务器信息", m_clntCcuCount);
	m_clntCcuConn = new TcpClient*[m_clntCcuCount];
	pthread_t tid;
	for(int i = 0; i < m_clntCcuCount; i++)
	{
		*(m_clntCcuConn + i) = new TcpClient;
		memset(*(m_clntCcuConn + i), 0, sizeof(TcpClient));
		(*(m_clntCcuConn + i))->initialize(g_vecCcuInfo[i]->ipaddr, atoi(transPort));
		pthread_mutex_t mutex;
		pthread_mutex_init(&mutex, NULL);
		g_mapRecvMessageLock.insert(pair<unsigned int, pthread_mutex_t>((*(m_clntCcuConn + i))->getInetAddr(), mutex));
		if((*(m_clntCcuConn + i))->connect_serv() > 0)
		{
			g_logs->WriteLog("与CCU[%s]服务通讯连接建立成功.", g_vecCcuInfo[i]->ipaddr);
		}
		else
		{
			g_logs->WriteWarn("连接CCU[%s]服务程序失败.", g_vecCcuInfo[i]->ipaddr);
		}
		pthread_create(&tid, NULL, thread_msg_recv, (void *)(*(m_clntCcuConn + i)));
	}
	pthread_create(&tid, NULL, thread_msg_send, this);

	// 连接到消息服务器,并订阅
	XmlCodec xml;
	m_subscribeLen[0] = xml.SubscribeMessage(m_subscribeBuf[0], 1);
	m_subscribeLen[1] = xml.SubscribeMessage(m_subscribeBuf[1], 2);
	m_msgServConn.initialize(ipaddr, atoi(port));
	if(m_msgServConn.connect_serv() > 0)
	{
		m_msgServConn.send_msg(m_subscribeBuf[0], m_subscribeLen[0]);
		m_msgServConn.send_msg(m_subscribeBuf[1], m_subscribeLen[1]);
		g_logs->WriteLog("消息服务器连接成功，消息订阅成功");
	}
	else
	{
		g_logs->WriteWarn("消息服务器连接失败, %s, %s", ipaddr, port);
	}

	// 启动消息服务器数据处理线程
	pthread_create(&tid, NULL, thread_sendXml, this);
	pthread_detach(tid);
	pthread_create(&tid, NULL, thread_recvXml, this);
	pthread_detach(tid);
	pthread_create(&tid, NULL, thread_dispatch, this);
	pthread_join(tid, NULL);
}

