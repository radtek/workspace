/*===================================================================================
 *	
 *	Copyright (C) 2018 by Jiaxing Shao. All rights reserved.
 *		
 * 	文 件 名: tcpclient.cpp
 * 	作    者: Jiaxing Shao
 * 	创建时间: 2018年08月13日
 *		
====================================================================================*/

#include "tcpclient.h"
#include "singleton_log.h"

TcpClient::TcpClient()
{
	m_nSockfd = 0;
	memset(m_szIpAddr, 0, IP_SIZE);
	pthread_mutex_init(&m_mutexLock, NULL);
	m_bConn = false;
}

TcpClient::~TcpClient()
{
	safe_close(m_nSockfd);
	pthread_mutex_destroy(&m_mutexLock);
}

void TcpClient::initialize(char *ipaddr, int port)
{
	memcpy(m_szIpAddr, ipaddr, IP_SIZE);
	m_nPort = port;
	m_inetAddr = inet_addr(m_szIpAddr);
}

int TcpClient::connect_serv()
{
	pthread_mutex_lock(&m_mutexLock);
	if(m_bConn)
	{
		pthread_mutex_unlock(&m_mutexLock);
		return m_nSockfd;
	}

	int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(sockfd == -1)
	{
		close(sockfd);
		g_logs->WriteWarn("socket() error, %s", strerror(errno));
		pthread_mutex_unlock(&m_mutexLock);
		return -2;
	}

	sockaddr_in serv_addr;
	socklen_t socklen = sizeof(sockaddr_in);
	memset(&serv_addr, 0, sizeof(sockaddr_in));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(m_szIpAddr);
	serv_addr.sin_port = htons(m_nPort);

	if(connect(sockfd, (sockaddr*)&serv_addr, socklen) == -1)
	{
		pthread_mutex_unlock(&m_mutexLock);
		close(sockfd);
		return -3;
	}
	m_bConn = true;
	m_nSockfd = sockfd;
	pthread_mutex_unlock(&m_mutexLock);
	return sockfd;
}

// 关闭套接字连接
void TcpClient::safe_close(int &fd)
{
	pthread_mutex_lock(&m_mutexLock);
	int ret = close(fd);
	while(ret != 0)
	{
		if(errno != EINTR || errno == EBADF)
		{
			break;
		}
		ret = close(fd);
	}
	m_bConn = false;
	pthread_mutex_unlock(&m_mutexLock);
}

// 返回值小于0需重连
int TcpClient::send_msg(const char *buffer, int length)
{
	if(buffer == NULL)
	{
		return 0;
	}

	if(!m_bConn)
	{
		return -1;
	}

	int n = send(m_nSockfd, buffer, length, 0);

	if(n < 0)
	{
		safe_close(m_nSockfd);
	}
	return n;
}

// 返回值非大于1时需重连
int TcpClient::recv_msg(char *buffer, int length)
{
	if(buffer == NULL || length <= 0)
	{
		return -1;
	}

RecvAgain:
	if(!m_bConn)
	{
		return -1;
	}

	int n = recv(m_nSockfd, buffer, length, 0);
	if(n == 0)
	{
		safe_close(m_nSockfd);
	}
	else if(n < 0)
	{
		if(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
		{
			goto RecvAgain;
		}
		safe_close(m_nSockfd);
	}
	return n;
}

