// =====================================================================================
//	Copyright (C) 2019 by Jiaxing Shao.All rights reserved.
//	文 件 名:  rtsp_server.cpp
//	作    者:  Jiaxing Shao, 13315567369@163.com
//	版 本 号:  1.0
//	创建时间:  2019年04月10日 10时13分41秒
//	Compiler:  g++
//	描    述:  
// =====================================================================================

#include "functions.h"
#include "logfile.h"
#include "rtsp_task.h"
#include "rtsp_client.h"
#include "rtsp_server.h"
#include "rtsp_protocol.h"
#include "threadpool.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <pthread.h>
#include <errno.h>

extern LOG_QUEUE *log_queue;
extern tcp_server_info *g_rtsp_serv;

int create_server_socket(int port)
{
	int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(sockfd == -1)
	{
		return -1;
	}

	struct sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(port);

	int opt = 1;
	if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (void *)&opt, sizeof(opt)) == -1)
	{
		return -1;
	}

	if(bind(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
	{
		return -1;
	}

	if(listen(sockfd, 5) == -1)
	{
		return -1;
	}

	return sockfd;
}

tcp_server_info *create_tcp_server(char *localip, char *port)
{
	int serv_port = atoi(port);
	int sockfd = create_server_socket(serv_port);
	if(sockfd == -1)
	{
		return NULL;
	}

	tcp_server_info *info = (tcp_server_info*)malloc(sizeof(tcp_server_info));
	for(int i = 0; i < MAX_DEVICE_COUNT; i++)
	{
		info->device[i] = (t_device_clnt*)malloc(sizeof(t_device_clnt));
		memset(info->device[i], 0, sizeof(t_device_clnt));
		info->device[i]->stop = true;
	}
	memcpy(info->ipaddr, localip, 16);
	info->sockfd = sockfd;
	info->port = serv_port;
	info->device_count = 0;
	return info;
}

void free_tcp_server(tcp_server_info* &info)
{
}

void rtsp_response(void *arg)
{
	int sockfd = *((int*)arg);

	char buffer[MAX_BUF_SIZE] = { 0 };
	int length = 0;
	length = recv_rtsp_message(sockfd, buffer, MAX_BUF_SIZE);
	if(length == -1)
	{
		close(sockfd);
	}
	int deviceid = rtsp_parse_cmd_options(buffer);
	t_device_video_play *player = video_task_get(deviceid);
	if(player == NULL)
	{
		close(sockfd);
		log_debug("no this deviceid %d", deviceid);
		return;
	}
	memset(buffer, 0, MAX_BUF_SIZE);
	length = rtsp_reply_options(player->vir_rtsp_info, buffer);
	send_rtsp_message(sockfd, buffer, length);

	while(true)
	{
		int n = recv_rtsp_command(player, sockfd);
		if(n == -1)
		{
			log_info(log_queue, "recv_rtsp_command %d failed.", n);
			return;
		}
		else if(n == 4)
		{
			log_info(log_queue, "rtsp process done.");
			break;
		}
		else if(n == 5)
		{
			return;
		}
	}
}

int recv_rtsp_command(t_device_video_play *player, int sockfd)
{
	char buffer[MAX_BUF_SIZE] = { 0 };
	int buflen = 0;

	// 接收cmd数据
	memset(buffer, 0, MAX_BUF_SIZE);
	for(int j = 0; j < 3; j++)
	{
		int len = recv(sockfd, buffer, MAX_BUF_SIZE - buflen, 0);
		if(len > 0)
		{
			buflen += len;
			if(buffer[buflen - 2] == '\r' && buffer[buflen - 1] == '\n')
			{
				break;
			}
		}

		if((errno != EINTR && errno != EWOULDBLOCK && errno != EAGAIN))
		{
			return -1;
		}
	}

	// 处理应答数据
	int type = rtsp_cmd_parse(player->vir_rtsp_info, buffer, buflen);

	memset(buffer, 0, MAX_BUF_SIZE);
	switch(type)
	{
		case 0:
			buflen = rtsp_reply_options(player->vir_rtsp_info, buffer);
			break;
		case 1:
			buflen = rtsp_reply_describe(player->vir_rtsp_info, buffer);
			break;
		case 2:
			buflen = rtsp_reply_setup(player->vir_rtsp_info, buffer, 1);
			break;
		case 3:
			buflen = rtsp_reply_setup(player->vir_rtsp_info, buffer, 2);
			break;
		case 4:
			buflen = rtsp_reply_play(player->vir_rtsp_info, buffer);
			break;
		case 5:
			buflen = rtsp_reply_teardown(player->vir_rtsp_info, buffer);
			break;
		default:
			break;
	}

	// 发送command数据
		return type;
}


