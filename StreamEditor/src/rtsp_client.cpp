// =====================================================================================
//	Copyright (C) 2019 by Jiaxing Shao.All rights reserved.
//	文 件 名:  rtsp_client.cpp
//	作    者:  Jiaxing Shao, 13315567369@163.com
//	版 本 号:  1.0
//	创建时间:  2019年04月02日 18时35分04秒
//	Compiler:  g++
//	描    述:  
// =====================================================================================

#include "rtsp_client.h"
#include "rtsp_util.h"
#include "rtsp_protocol.h"
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
	memset(player->dev_rtsp_info->video_url, 0, 128);
	memset(player->dev_rtsp_info->audio_url, 0, 128);
	memset(player->dev_rtsp_info->session, 0, 64);
	memset(player->dev_rtsp_info->nonce, 0, 64);
	memset(player->dev_rtsp_info->realm, 0, 32);
	player->dev_rtsp_info->cmd_seq = 0;
	player->dev_rtsp_info->secret = false;
	player->dev_rtsp_info->step = enum_cmd_options;
	player->dev_rtsp_info->counter = 0;

	char buffer[MAX_BUF_SIZE] = { 0 };
	int length = 0;
	int result = 0;
	while(true)
	{
		memset(buffer, 0, MAX_BUF_SIZE);
		switch(player->dev_rtsp_info->step)
		{
			case enum_cmd_options:
				length = rtsp_cmd_options(player->dev_rtsp_info, buffer);
				break;
			case enum_cmd_describe:
				length = rtsp_cmd_describe(player->dev_rtsp_info, buffer);
				break;
			case enum_cmd_setup:
				length = rtsp_cmd_setup(player->dev_rtsp_info, buffer);
				break;
			case enum_cmd_play:
				length = rtsp_cmd_play(player->dev_rtsp_info, buffer);
				break;
			default:
				length = -1;
				break;
		}
		if(length == -1)
		{
			result = -1;
			break;
		}

		int n = send_rtsp_message(player->sockfd, buffer, length);
		memset(buffer, 0, MAX_BUF_SIZE);
		length = recv_rtsp_message(player->sockfd, buffer, MAX_BUF_SIZE);

		switch(player->dev_rtsp_info->step)
		{
			case enum_cmd_options:
				result = rtsp_parse_reply_options(player->dev_rtsp_info, buffer, length);
				break;
			case enum_cmd_describe:
				result = rtsp_parse_reply_describe(player->dev_rtsp_info, buffer, length);
				break;
			case enum_cmd_setup:
				result = rtsp_parse_reply_setup(player->dev_rtsp_info, buffer, length);
				break;
			case enum_cmd_play:
				result = rtsp_parse_reply_play(player->dev_rtsp_info, buffer, length);
				break;
			default:
				result = -1;
				break;
		}

		// result为1时,握手结束
		if(result == 0)
		{
			if(player->dev_rtsp_info->step == enum_cmd_setup &&
				player->dev_rtsp_info->counter == 0)
			{
				player->dev_rtsp_info->step = enum_cmd_setup;
			}
			else
			{
				player->dev_rtsp_info->step += 1;
			}
		}
		else if(result == 1)
		{
			if(player->dev_rtsp_info->step == enum_cmd_describe)
			{
				player->dev_rtsp_info->step = enum_cmd_describe;
			}
			else
			{
				break;
			}
		}
		else
		{
			log_debug("client send rtsp message error, message:\n"
					"%s\n", buffer);
			break;
		}
	}
	
	if(result == 1)
	{
		memcpy(player->vir_rtsp_info->session, player->dev_rtsp_info->session, 64);
		memcpy(player->vir_rtsp_info->ssrc[0], player->dev_rtsp_info->ssrc[0], 16);
		memcpy(player->vir_rtsp_info->ssrc[1], player->dev_rtsp_info->ssrc[1], 16);
		memcpy(player->vir_rtsp_info->username, player->dev_rtsp_info->username, 32);
		memcpy(player->vir_rtsp_info->password, player->dev_rtsp_info->password, 32);
		memcpy(player->vir_rtsp_info->nonce, player->dev_rtsp_info->nonce, 64);
		memcpy(player->vir_rtsp_info->realm, player->dev_rtsp_info->nonce, 64);
		return true;
	}
	return false;
}

