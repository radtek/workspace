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
	FD_ZERO(&info->fds);
	FD_SET(sockfd, &info->fds);
	info->maxfd = sockfd;
	memset(info->vir_url, 0, 128);
	sprintf(info->vir_url, "rtsp://%s:%d/video/h264/", info->ipaddr, info->port);
	return info;
}

void free_tcp_server(tcp_server_info* &info)
{
}

void rtsp_response(void *arg)
{
	tcp_client_info *clnt = (tcp_client_info*)arg;

	char buffer[MAX_BUF_SIZE] = { 0 };
	int length = 0;
	length = recv_rtsp_message(clnt->sockfd, buffer, MAX_BUF_SIZE);
	if(length == -1)
	{
		log_debug("接收RTSP请求数据失败, %d.", length);
		log_info(log_queue, "接收RTSP请求数据失败, %d, 客户端地址 %s.", length, clnt->ipaddr);
		close(clnt->sockfd);
		return;
	}

	int deviceid = 0;
	int seq = 0;
	int over = -1;

	seq = rtsp_parse_cmd_options(g_rtsp_serv->vir_url, buffer, deviceid);
	if(seq == -1)
	{
		log_debug("错误的RTSP连接, 连接断开.");
		log_info(log_queue, "错误的RTSP连接, 连接断开.");
		close(clnt->sockfd);
		return;
	}

	t_device_video_play *player = video_task_get(deviceid);
	if(player == NULL)
	{
		close(clnt->sockfd);
		log_debug("所请求的设备ID不存在, %d", deviceid);
		log_info(log_queue, "所请求的设备ID不存在, %d", deviceid);
		return;
	}
	else
	{
		if(player->stop)
		{
			log_debug("错误的连接请求,请先订阅, 设备ID %d", deviceid);
			log_info(log_queue, "错误的服务连接请求,请先订阅此设备视频服务, 设备ID %d", deviceid);
			close(clnt->sockfd);
			return;
		}
		log_debug("request video stream, deviceid %d, sockfd %d", deviceid, clnt->sockfd);
	}
	memset(buffer, 0, MAX_BUF_SIZE);
	length = rtsp_reply_options(player->vir_rtsp_info, buffer, seq, over);
	log_info(log_queue, "设备ID %d, 返回 OPTIONS 应答消息", deviceid);
	send_rtsp_message(clnt->sockfd, buffer, length);
	int step = 1;

	t_rtsp_info *tmp_info = (t_rtsp_info*)malloc(sizeof(t_rtsp_info));
	memcpy(tmp_info, player->vir_rtsp_info, sizeof(t_rtsp_info));

	while(true)
	{
		memset(buffer, 0, MAX_BUF_SIZE);
		length = recv_rtsp_message(clnt->sockfd, buffer, MAX_BUF_SIZE);
		if(length == -1)
		{
			log_debug("recv_rtsp_command %d failed.", step);
			log_info(log_queue, "接收RTSP请求数据失败, 客户端地址 %s.", clnt->ipaddr);
			close(clnt->sockfd);
			free(tmp_info);
			return;
		}

		switch(step)
		{
		case enum_cmd_describe:
			seq = rtsp_parse_cmd_describe(tmp_info, buffer, length);
			memset(buffer, 0, MAX_BUF_SIZE);
			length = rtsp_reply_describe(tmp_info, buffer, seq, over);
			log_info(log_queue, "设备ID %d, 返回 DESCRIBE 应答消息", deviceid);
			break;
		case enum_cmd_setup:
			seq = rtsp_parse_cmd_setup(tmp_info, buffer, length);
			memset(buffer, 0, MAX_BUF_SIZE);
			length = rtsp_reply_setup(tmp_info, buffer, seq, over);
			log_info(log_queue, "设备ID %d, 返回 SETUP 应答消息", deviceid);
			break;
		case enum_cmd_play:
			seq = rtsp_parse_cmd_play(tmp_info, buffer, length);
			memset(buffer, 0, MAX_BUF_SIZE);
			length = rtsp_reply_play(tmp_info, buffer, seq, over);
			log_info(log_queue, "设备ID %d, 返回 PLAY 应答消息", deviceid);
			break;
		case enum_cmd_teardown:
			seq = rtsp_parse_cmd_teardown(tmp_info, buffer, length);
			memset(buffer, 0, MAX_BUF_SIZE);
			length = rtsp_reply_teardown(tmp_info, buffer, seq, over);
			log_info(log_queue, "设备ID %d, 返回 TEARDOWN 应答消息", deviceid);
			break;
		default:
			length = -1;
			break;
		}
		if(length == -1)
		{
			break;
		}

		send_rtsp_message(clnt->sockfd, buffer, length);
		if(step == enum_cmd_play)
		{
			break;
		}

		if(over == 0)
		{
			step += 1;
		}
	}

	if(length == -1)
	{
		close(clnt->sockfd);
		log_info(log_queue, "客户端连接失败, 设备ID %d, 客户端地址 %s.", deviceid, clnt->ipaddr);
	}
	else
	{
		pthread_mutex_lock(&g_rtsp_serv->lock);
		int count = g_rtsp_serv->device[player->serv_pos]->clnt_count;
		if(count == MAX_CLIENT_COUNT)
		{
			pthread_mutex_unlock(&g_rtsp_serv->lock);
			close(clnt->sockfd);
			log_info(log_queue, "客户端 %s 连接失败,当前设备连接数超过最大限制, 可接受连接数 %d", clnt->ipaddr, MAX_CLIENT_COUNT);
			return;
		}
		g_rtsp_serv->device[player->serv_pos]->clntfd[count] = clnt->sockfd;
		g_rtsp_serv->device[player->serv_pos]->clnt_count++;
		g_rtsp_serv->device[player->serv_pos]->time_count = 0;
		FD_SET(clnt->sockfd, &g_rtsp_serv->fds);
		if(g_rtsp_serv->maxfd < clnt->sockfd)
		{
			g_rtsp_serv->maxfd = clnt->sockfd;
		}
		log_info(log_queue, "客户端 %s 连接成功,设备ID %d, 当前连接数 %d.", clnt->ipaddr, deviceid, g_rtsp_serv->device[player->serv_pos]->clnt_count);
		pthread_mutex_unlock(&g_rtsp_serv->lock);
	}
	free(tmp_info);
}

