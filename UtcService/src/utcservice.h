// =====================================================================================
//	Copyright (C) 2018 by Jiaxing Shao.All rights reserved.
//	文 件 名:  utcservice.h
//	作    者:  Jiaxing Shao, 13315567369@163.com
//	版 本 号:  1.0
//	创建时间:  2018年08月13日 18时06分31秒
//	Compiler:  g++
//	描    述:  
// =====================================================================================

#ifndef __UTCSERVICE_H_H_H
#define __UTCSERVICE_H_H_H

#include "functions.h"
#include "tcpclient.h"
#include "threadpool.h"
#include "otlcontrol.h"
#include <vector>

class UtcService
{
public:
	UtcService();
	~UtcService();
	void Start();
	static void *thread_sendXml(void *arg);
	static void *thread_recvXml(void *arg);
	static void *thread_msg_send(void *arg);
	static void *thread_msg_recv(void *arg);
	static void *thread_dispatch(void *arg);
private:
	ThreadPool m_threadpool;		// 线程池
	OtlControl m_otlDatabase;

	TcpClient m_msgServConn;			// 消息服务器
	char m_subscribeBuf[3][BUF_LEN];	// 要订阅的消息
	int m_subscribeLen[3];				// 订阅消息长度
	TcpClient **m_clntCcuConn;			// ccu连接
	int m_clntCcuCount;					// ccu个数
};

#endif

