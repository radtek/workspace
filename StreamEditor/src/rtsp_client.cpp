// =====================================================================================
//	Copyright (C) 2019 by Jiaxing Shao.All rights reserved.
//	文 件 名:  rtsp_client.cpp
//	作    者:  Jiaxing Shao, 13315567369@163.com
//	版 本 号:  1.0
//	创建时间:  2019年04月02日 18时35分04秒
//	Compiler:  g++
//	描    述:  
// =====================================================================================

#include "bytearray.h"
#include "rtsp_client.h"
#include "rtsp_protocol.h"
#include "logfile.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

extern LOG_QUEUE *log_queue;

tcp_conn_info *create_tcp_client_conn(char *ipaddr, int port)
{
	tcp_conn_info *clnt = new tcp_conn_info;
	memset(clnt, 0, sizeof(tcp_conn_info));
	memcpy(clnt->ipaddr, ipaddr, 16);
	clnt->port = port;
	clnt->stop = false;

	do{
		clnt->sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
		if(clnt->sockfd == -1)
		{
			log_info(log_queue, "%s[%05d]: socket() failed.", __FILE__, __LINE__);
			break;
		}

		struct sockaddr_in sock_addr;
		memset(&sock_addr, 0, sizeof(sock_addr));
		sock_addr.sin_family = AF_INET;
		sock_addr.sin_addr.s_addr = inet_addr(ipaddr);
		sock_addr.sin_port = htons(port);

		if(connect(clnt->sockfd, (struct sockaddr*)&sock_addr, sizeof(sockaddr_in)) == -1)
		{
			log_info(log_queue, "%s[%05d]: connect() failed.", __FILE__, __LINE__);
			break;
		}
		return clnt;
	}while(0);
	
	if(clnt != NULL)
	{
		delete clnt;
		clnt = NULL;
	}
	return NULL;
}

bool rtsp_request(t_video_play_info *player)
{
	for(int i = 0; i < 6; i++)
	{
		if(!send_rtsp_command(player, i))
		{
			log_info(log_queue, "send_rtsp_command %d failed.", i);
			return false;
		}
	}
	return true;
}

void *rtsp_worker_start(void *arg)
{
	return NULL;
}

/*
void rtsp_worker_start(tcp_conn_info *clnt, char *url, char *username, char *password)
{
	rtsp_info *info = new rtsp_info;
	info->cmd_seq = 0;
	info->secret = false;
	rtsp_init(info, url, username, password);

ReConnectRtsp:
	clnt->sockfd = connect_serv(clnt->ipaddr, clnt->port);
	if(clnt->sockfd == -1)
	{
		log_info(log_queue, "connect server [%s:%d] failed.", clnt->ipaddr, clnt->port);
		return;
	}

	do{
		bool isFail = false;
		for(int i = 0; i < 6; i++)
		{
			if(!send_rtsp_command(clnt, info, i))
			{
				log_info(log_queue, "send_rtsp_command %d failed.", i);
				isFail = true;
				break;
			}
		}

		if(isFail)
		{
			break;
		}

		ByteArray *rtpArray = new ByteArray();
		pthread_t pid = 0;
		//pthread_create(&pid, NULL, rtp_parse_thread, (void*)rtpArray);

		char buffer[2048] = { 0 };
		int buflen = 0;
		while(true)
		{
			if(clnt->stop)
			{
				send_rtsp_command(clnt, info, 7);
				close(clnt->sockfd);
				break;
			}

			buflen = recv(clnt->sockfd, buffer, 2048, 0);
			if(buflen <= 0)
			{
				if(errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
				{
					continue;
				}
				close(clnt->sockfd);
				break;
			}
			else
			{
				rtpArray->put_message(buffer, buflen);
			}
		}

		if(pthread_cancel(pid) != 0)
		{
			log_info(log_queue, "pthread_cancel() error!");
		}

		if(rtpArray != NULL)
		{
			delete rtpArray;
			rtpArray = NULL;
		}
	}while(0);

	if(clnt != NULL)
	{
		delete clnt;
		clnt = NULL;
	}
	if(info != NULL)
	{
		delete info;
		info = NULL;
	}
}
*/

void rtsp_worker_stop(tcp_conn_info *clnt)
{
	clnt->stop = true;
}

bool send_rtsp_command(t_video_play_info *player, int type)
{
	char buffer[1024] = { 0 };
	int buflen = 0;
	switch(type)
	{
		case enum_cmd_options:
			buflen = rtsp_cmd_options(player->rtsp_info, buffer);
			break;
		case enum_cmd_describe:
			buflen = rtsp_cmd_describe(player->rtsp_info, buffer);
			break;
		case enum_cmd_describe_secret:
			buflen = rtsp_cmd_describe(player->rtsp_info, buffer);
			break;
		case enum_cmd_setup_video:
			buflen = rtsp_cmd_setup(player->rtsp_info, buffer, 1);
			break;
		case enum_cmd_setup_audio:
			buflen = rtsp_cmd_setup(player->rtsp_info, buffer, 2);
			break;
		case enum_cmd_play:
			buflen = rtsp_cmd_play(player->rtsp_info, buffer);
			break;
		case enum_cmd_teardown:
			buflen = rtsp_cmd_teardown(player->rtsp_info, buffer);
			break;
		default:
			break;
	}
	int len = 0;

	// 发送command数据
	for(int i = 0; i < 3; i++)
	{
		len = send(player->device_conn->sockfd, buffer, buflen, 0);
		if(len > 0)
		{
			break;
		}

		if((errno != EINTR && errno != EWOULDBLOCK && errno != EAGAIN))
		{
			return false;
		}
	}

	// 接收reply数据
	memset(player->device_conn->buffer, 0, MAX_BUF_SIZE);
	for(int j = 0; j < 3; j++)
	{
		len = recv(player->device_conn->sockfd, player->device_conn->buffer, MAX_BUF_SIZE, 0);
		if(len > 0)
		{
			break;
		}

		if((errno != EINTR && errno != EWOULDBLOCK && errno != EAGAIN))
		{
			return false;
		}
	}

	switch(type)
	{
		case enum_cmd_options:
			player->reply_options = player->device_conn->buffer;
			break;
		case enum_cmd_describe:
			player->reply_describe[0] = player->device_conn->buffer;
			break;
		case enum_cmd_describe_secret:
			player->reply_describe[1] = player->device_conn->buffer;
			break;
		case enum_cmd_setup_video:
			player->reply_setup[0] = player->device_conn->buffer;
			break;
		case enum_cmd_setup_audio:
			player->reply_setup[1] = player->device_conn->buffer;
			break;
		case enum_cmd_play:
			player->reply_play = player->device_conn->buffer;
			break;
		case enum_cmd_teardown:
			player->reply_teardown = player->device_conn->buffer;
			break;
	}

	// 处理应答数据
	rtsp_reply_parse(player->rtsp_info, player->device_conn->buffer, len, type);
	return true;
}

/*
void *rtp_parse_thread(void *arg)
{
	ByteArray *rtpArray = (ByteArray*)arg;
	char buffer[2048] = { 0 };
	int buflen = 0;
	while(true)
	{
		memset(buffer, 0, 2048);
		while(true)
		{
			if(rtpArray->get_message(buffer, 4, true, false))
			{
				break;
			}
		}
		int buflen = ((buffer[2] & 0xFF) * 256) + (buffer[3] & 0xFF);
		while(true)
		{
			if(rtpArray->get_message(buffer + 4, buflen, true, false))
			{
				break;
			}
		}
	}
}
*/
