// =====================================================================================
//	Copyright (C) 2019 by Jiaxing Shao.All rights reserved.
//	文 件 名:  rtsp_client.cpp
//	作    者:  Jiaxing Shao, 13315567369@163.com
//	版 本 号:  1.0
//	创建时间:  2019年04月02日 18时35分04秒
//	Compiler:  g++
//	描    述:  
// =====================================================================================

#include "rtsp_task.h"
#include "rtsp_client.h"
#include "rtsp_protocol.h"
#include "bytearray.h"
#include "logfile.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

extern LOG_QUEUE *log_queue;
extern tcp_server_info *g_rtsp_serv;

int connect_server(char *ipaddr, int port)
{
	do{
		int sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
		if(sockfd == -1)
		{
			log_info(log_queue, "%s[%05d]: socket() failed.", __FILE__, __LINE__);
			break;
		}

		int recv_buf_size = 1024 * 1024 * 10;
		if(setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (char*)&recv_buf_size, sizeof(int)))
		{
			log_info(log_queue, "%s[%05d]: setsockopt() failed.", __FILE__, __LINE__);
			break;
		}

		struct sockaddr_in sock_addr;
		memset(&sock_addr, 0, sizeof(sock_addr));
		sock_addr.sin_family = AF_INET;
		sock_addr.sin_addr.s_addr = inet_addr(ipaddr);
		sock_addr.sin_port = htons(port);

		// 连接到设备
		if(connect(sockfd, (struct sockaddr*)&sock_addr, sizeof(sockaddr_in)) == -1)
		{
			log_info(log_queue, "%s[%05d]: connect() failed.", __FILE__, __LINE__);
			break;
		}
		return sockfd;
	}while(0);
	return -1;
}

bool rtsp_request(t_device_video_play *player)
{
	for(int i = 0; i < 6; i++)
	{
		if(!send_rtsp_command(player, i))
		{
			log_debug("send rtsp command %d failed", i);
			log_info(log_queue, "send_rtsp_command %d failed.", i);
			return false;
		}
	}
	
	memcpy(player->vir_rtsp_info->session, player->dev_rtsp_info->session, 64);
	memcpy(player->vir_rtsp_info->ssrc[0], player->dev_rtsp_info->ssrc[0], 16);
	memcpy(player->vir_rtsp_info->ssrc[1], player->dev_rtsp_info->ssrc[1], 16);
	memcpy(player->vir_rtsp_info->username, player->dev_rtsp_info->username, 32);
	memcpy(player->vir_rtsp_info->password, player->dev_rtsp_info->password, 32);
	memcpy(player->vir_rtsp_info->nonce, player->dev_rtsp_info->nonce, 64);
	memcpy(player->vir_rtsp_info->realm, player->dev_rtsp_info->nonce, 64);
	return true;
}

bool send_rtsp_command(t_device_video_play *player, int type)
{
	char buffer[MAX_BUF_SIZE] = { 0 };
	int buflen = 0;
	switch(type)
	{
		case enum_cmd_options:
			buflen = rtsp_cmd_options(player->dev_rtsp_info, buffer);
			break;
		case enum_cmd_describe:
			buflen = rtsp_cmd_describe(player->dev_rtsp_info, buffer);
			break;
		case enum_cmd_describe_secret:
			buflen = rtsp_cmd_describe(player->dev_rtsp_info, buffer);
			break;
		case enum_cmd_setup_video:
			buflen = rtsp_cmd_setup(player->dev_rtsp_info, buffer, 1);
			break;
		case enum_cmd_setup_audio:
			buflen = rtsp_cmd_setup(player->dev_rtsp_info, buffer, 2);
			break;
		case enum_cmd_play:
			buflen = rtsp_cmd_play(player->dev_rtsp_info, buffer);
			break;
		case enum_cmd_teardown:
			buflen = rtsp_cmd_teardown(player->dev_rtsp_info, buffer);
			break;
		default:
			break;
	}

	// 发送command数据
	for(int i = 0; i < 3; i++)
	{
		int n = send(player->sockfd, buffer, buflen, 0);
		if(n > 0)
		{
			break;
		}

		if((errno != EINTR && errno != EWOULDBLOCK && errno != EAGAIN))
		{
			log_debug("device send rtsp message error, %d", n);
			return false;
		}
	}

	// 接收reply数据
	memset(buffer, 0, MAX_BUF_SIZE);
	int length = 0;
	for(int j = 0; j < 3; j++)
	{
		int n = recv(player->sockfd, buffer + length, MAX_BUF_SIZE - length, 0);
		if(n > 0)
		{
			length += n;
			if(buffer[n - 2] == '\r' && buffer[n - 1] == '\n')
			{
				break;
			}
		}

		if((errno != EINTR && errno != EWOULDBLOCK && errno != EAGAIN))
		{
			log_debug("device recv rtsp message error, %d", n);
			return false;
		}
	}

	// 处理应答数据
	return rtsp_reply_parse(player->dev_rtsp_info, buffer, length, type);
}


