/*===================================================================================
 *	
 *	Copyright (C) 2018 by Jiaxing Shao. All rights reserved.
 *		
 * 	文 件 名: tcpclient.h
 * 	作    者: Jiaxing Shao
 * 	创建时间: 2018年08月13日
 *		
====================================================================================*/

#ifndef _TCPCLIENT_H_H_H
#define _TCPCLIENT_H_H_H

#include "defines.h"
#include <iostream>
using namespace std;
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

class TcpClient
{
public:
	TcpClient();
	virtual ~TcpClient();
	int send_msg(const char *buffer, int length);
	int recv_msg(char *buffer, int length);
	void initialize(char *ipaddr, int port);
	int connect_serv();
	bool getConnStat()
	{
		bool result;
		pthread_mutex_lock(&m_mutexLock);
		result = m_bConn;
		pthread_mutex_unlock(&m_mutexLock);
		return result;
	}
	unsigned int getInetAddr()
	{
		return m_inetAddr;
	}

public:
	char m_szIpAddr[IP_SIZE];
	int m_nPort;

private:
	void safe_close(int &fd);

private:
	bool m_bConn;
	int m_nSockfd;
	unsigned int m_inetAddr;
	pthread_mutex_t m_mutexLock;
};

#endif

