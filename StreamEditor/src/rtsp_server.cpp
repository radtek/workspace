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
#include "rtsp_util.h"
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
	int deviceid = 0;
	int seq = 0;
	seq = rtsp_parse_cmd_options(buffer, deviceid);
	t_device_video_play *player = video_task_get(deviceid);
	if(player == NULL)
	{
		close(sockfd);
		log_debug("no this deviceid %d", deviceid);
		return;
	}
	memset(buffer, 0, MAX_BUF_SIZE);
	length = rtsp_reply_options(player->vir_rtsp_info, buffer, seq);
	send_rtsp_message(sockfd, buffer, length);
	int step = 1;

	while(true)
	{
		memset(buffer, 0, MAX_BUF_SIZE);
		length = recv_rtsp_message(sockfd, buffer, MAX_BUF_SIZE);
		if(length == -1)
		{
			log_info(log_queue, "recv_rtsp_command %d failed.", step);
			return;
		}

		switch(step)
		{
		case enum_cmd_describe:
			seq = rtsp_parse_cmd_describe(player->vir_rtsp_info, buffer, length);
			length = rtsp_reply_describe(player->vir_rtsp_info, buffer, seq);
			break;
		case enum_cmd_setup:
			seq = rtsp_parse_cmd_setup(player->vir_rtsp_info, buffer, length);
			length = rtsp_reply_setup(player->vir_rtsp_info, buffer, seq);
			break;
		case enum_cmd_play:
			seq = rtsp_parse_cmd_play(player->vir_rtsp_info, buffer, length);
			length = rtsp_reply_play(player->vir_rtsp_info, buffer, seq);
			break;
		case enum_cmd_teardown:
			seq = rtsp_parse_cmd_teardown(player->vir_rtsp_info, buffer, length);
			length = rtsp_reply_teardown(player->vir_rtsp_info, buffer, seq);
			break;
		default:
			break;
		}
		send_rtsp_message(sockfd, buffer, length);
		if(step == enum_cmd_play)
		{
			break;
		}
	}

	pthread_mutex_lock(&g_rtsp_serv->lock);
	int count = g_rtsp_serv->device[player->serv_pos]->clnt_count;
	if(count == MAX_CLIENT_COUNT)
	{
		pthread_mutex_unlock(&g_rtsp_serv->lock);
		close(sockfd);
		log_debug("rtsp请求连接数超过限制, max client %d", MAX_CLIENT_COUNT);
	}
	g_rtsp_serv->device[player->serv_pos]->clntfd[count] = sockfd;
	g_rtsp_serv->device[player->serv_pos]->clnt_count++;
	FD_SET(sockfd, &g_rtsp_serv->fds);
	if(g_rtsp_serv->maxfd < sockfd)
	{
		g_rtsp_serv->maxfd = sockfd;
	}
	pthread_mutex_unlock(&g_rtsp_serv->lock);
}

