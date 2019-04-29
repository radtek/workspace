/*===================================================================================
 *	
 *	Copyright (C) 2018 by Jiaxing Shao. All rights reserved.
 *		
 * 	文 件 名: tcpclient.cpp
 * 	作    者: Jiaxing Shao
 * 	创建时间: 2018年05月21日
 *		
====================================================================================*/

#include "tcpclient.h"

TcpClient::TcpClient()
{
	m_nSockfd = 0;
	m_isConn = false;
	memset(m_szIpaddr, 0, IP_SIZE);
	memset(m_szPort, 0, PORT_SIZE);
	memset(&m_version, 0, sizeof(T_LicenceVersion));
	pthread_mutex_init(&m_mutexLock, NULL);
	pthread_cond_init(&m_condData, NULL);
	pthread_mutex_init(&m_mutexData, NULL);
}

TcpClient::~TcpClient()
{
	pthread_mutex_destroy(&m_mutexLock);
	pthread_cond_destroy(&m_condData);
	pthread_mutex_destroy(&m_mutexData);
}

int TcpClient::Connect(char *ipaddr, char *port)
{
	int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(sock == -1)
	{
		g_logs->WriteLog("socket() 调用失败.");
		return -1;
	}

	sockaddr_in serv_addr;
	socklen_t socklen =  sizeof(sockaddr_in);
	memset(&serv_addr,0,sizeof(sockaddr_in));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(ipaddr);
	serv_addr.sin_port = htons(atoi(port));

	if(connect(sock, (sockaddr*)&serv_addr, socklen) == -1)
	{
		g_logs->WriteLog("connect() 调用失败.");
		sleep(1);
		return -2;
	}

	if(LoginScats(sock))
	{
		m_isConn = true;
		LogInfo.WriteLog("已连接到 scats系统并登录成功。");
	}
	else
	{
		LogInfo.WriteLog("连接scats系统失败。");
		return -1;
	}
	return sock;
}

void TcpClient::safe_close(int fd)
{
	int ret = close(fd);
	while(ret != 0)
	{
		if(errno != EINTR || errno == EBADF)
		{
			break;
		}
		ret = close(fd);
	}
}

void* TcpClient::pthread_recv(void *arg)
{
	TcpClient *pthis = (TcpClient*)arg;

	char buffer[RCVBUF_SIZE] = {0};
	int length = 0;
	bool result = false;
	while(true)
	{
		length = 0;
		memset(buffer, 0, RCVBUF_SIZE);

		if(pthis->m_isConn)
		{
			length = recv(pthis->m_nSockfd, buffer, RCVBUF_SIZE, 0);
			if(length == 0)
			{
DisConnect:
				pthread_mutex_lock(&pthis->m_mutexLock);
				pthis->safe_close(pthis->m_nSockfd);
				pthis->m_isConn = false;
				while(!pthis->m_isConn)
				{
					sleep(3);
					pthis->m_nSockfd = pthis->Connect(pthis->m_szIpaddr, pthis->m_szPort);
				}
				pthread_mutex_unlock(&pthis->m_mutexLock);
				continue;
			}
			else if(length < 0)
			{
				if(errno == EWOULDBLOCK || errno == EINTR || errno == EAGAIN)
				{
					continue;
				}
				goto DisConnect;
			}

			// 心跳包不予处理
			if(length == 3 && buffer[2] == 0x00)
				continue;
			pthread_mutex_lock(&pthis->m_mutexData);
			result = pthis->m_szBuffer.put_message(buffer, length);
			pthread_mutex_unlock(&pthis->m_mutexData);
			if(result)
			{
				pthread_cond_signal(&pthis->m_condData);
			}
			else
			{
				char temp[RCVBUF_SIZE] = {0};
				int len = ParseMessageHex(temp, buffer, length);
				LogInfo.WriteLog("put_message error: %s", temp);
			}
		}
		else
		{
			sleep(3);
		}
	}
}

void *TcpClient::pthread_parse(void *arg)
{
	TcpClient *pthis = (TcpClient*)arg;
	char buffer[RCVBUF_SIZE] = { 0 };
	char bufhex[RCVBUF_SIZE * 3] = { 0 };
	int length = 0;
	while(true)
	{
		memset(buffer, 0, RCVBUF_SIZE);
		pthread_mutex_lock(&pthis->m_mutexData);
		while(pthis->m_szBuffer.get_count() == 0)
		{
			pthread_cond_wait(&pthis->m_condData, &pthis->m_mutexData);
		}
		bool result = pthis->m_szBuffer.get_message(buffer, length);
		if(result)
		{
			memset(bufhex, 0, RCVBUF_SIZE * 3);
			ParseMessageHex(bufhex, buffer, length);
			g_logs->WriteLog("Recv Message: %s", bufhex);
			pthis->m_scats.Translate(buffer, length);
		}
		pthread_mutex_unlock(&pthis->m_mutexData);
	}
}

void TcpClient::Start(char *ipaddr, char *port)
{
	memcpy(m_szIpaddr, ipaddr, IP_SIZE);
	memcpy(m_szPort, port, PORT_SIZE);

	while(!m_isConn)
	{
		m_nSockfd = Connect(m_szIpaddr,m_szPort);
		if(m_nSockfd <= 0)
		{
			sleep(5);
		}
	}

	pthread_t tid[2];
	pthread_create(&tid[0], NULL, pthread_recv, this);
	pthread_detach(tid[0]);
	pthread_create(&tid[1], NULL, pthread_parse, this);
	pthread_detach(tid[1]);

	int command = 0;
	char buffer[RCVBUF_SIZE] = { 0 };
	char bufhex[RCVBUF_SIZE * 3] = { 0 };
	string str;
	int length = 0;

	cout << "连接成功，可输入命令发送指定数据包" << endl;
	while(true)
	{
		getline(cin, str);
		command = atoi(str.c_str());

		length = m_scats.test_func(buffer, command);
		if(length > 0)
		{
			int n = send(m_nSockfd, buffer, length, 0);
			if(n > 0)
			{
				memset(bufhex, 0, RCVBUF_SIZE * 3);
				ParseMessageHex(bufhex, buffer, n);
				g_logs->WriteLog("Send Message: %s", bufhex);
			}
		}
	}
}

bool TcpClient::LoginScats(int sock)
{
	char buffer[1024] = {0};
	int length = 0;
	memset(buffer, 0, 1024);
	length = m_scats.EncodeLicenceMsg(buffer);

	if(send(sock, buffer, length, 0) < 0)
	{
		safe_close(sock);
		return false;
	}
	memset(buffer, 0, 1024);
	if(recv(sock, buffer, 1024, 0) <= 0)
	{
		safe_close(sock);
		return false;
	}
	if(!m_scats.DecodeResLicenceMsg(buffer, m_version))
	{
		safe_close(sock);
		return false;
	}

	/*
	memset(buffer, 0, 1024);
	length = m_scats.EncodeUserVerify(buffer);

	if(send(sock, buffer, length, 0) < 0)
	{
		safe_close(sock);
		return false;
	}
	memset(buffer, 0, 1024);
	if(recv(sock, buffer, 1024, 0) <= 0)
	{
		safe_close(sock);
		return false;
	}
	if(!m_scats.DecodeResUserVerify(buffer))
	{
		safe_close(sock);
		return false;
	}
	*/
	return true;
}
