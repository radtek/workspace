/*===================================================================================
 *	
 *	Copyright (C) 2018 by Jiaxing Shao. All rights reserved.
 *		
 * 	文 件 名: tcpclient.h
 * 	作    者: Jiaxing Shao
 * 	创建时间: 2018年05月21日
 *		
====================================================================================*/

#ifndef _TCPCLIENT_H_H_H
#define _TCPCLIENT_H_H_H

#include "log.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <stdlib.h>
#include <errno.h>

#include "bytearray.h"
#include "protocol_scats.h"

#define IP_SIZE			16
#define PORT_SIZE		8

class TcpClient
{
public:
	TcpClient();
	virtual ~TcpClient();
	void Start(char *ipaddr, char *port);
public:
	static void* pthread_recv(void *arg);
	static void* pthread_parse(void *arg);
private:
	int Connect(char *ipaddr, char *port);
	bool LoginScats(int sock);
	void safe_close(int fd);
private:
	pthread_mutex_t m_mutexLock;

	pthread_cond_t m_condData;
	pthread_mutex_t m_mutexData;

	char m_szIpaddr[IP_SIZE];
	char m_szPort[PORT_SIZE];
	bool m_isConn;
	int m_nSockfd;

	ByteArray m_szBuffer;		// 消息队列，存储接收到的数据包
	ProtocolScats m_scats;		// 协议解析，组包
	ThreadPool m_pool;			// 线程池

	T_LicenceVersion m_version;
};

#endif
