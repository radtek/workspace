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

tcp_conn_info *create_tcp_client_conn(char *ipaddr, int port)
{
	tcp_conn_info *clnt = new tcp_conn_info;
	memset(clnt, 0, sizeof(tcp_conn_info));
	memcpy(clnt->ipaddr, ipaddr, 16);
	clnt->port = port;

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

		// 连接到设备
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


void *rtsp_worker_start(void *arg)
{
	int deviceid = *((int*)arg);
	log_debug("rtsp_worker_start 线程启动, deviceid %d", deviceid);
	pthread_detach(pthread_self());
	t_video_play_info *player = video_task_get(deviceid);
	if(player == NULL)
	{
		log_debug("rtsp_worker_start 获取 player 失败, deviceid %d, 线程退出", deviceid);
	}
	char buffer[MAX_VIDEO_CACHE] = { 0 };
	int sockfd = player->device_conn->sockfd;

	while(true)
	{
		if(player->stop)
		{
			log_debug("rtsp_worker_start 线程准备退出, deviceid %d", deviceid);
			break;
		}

		int n = recv(player->device_conn->sockfd, buffer, MAX_VIDEO_CACHE, 0);
		if(n > 0)
		{
			put_byte_array(player->rtp_array, buffer, n);
		}
		else if(n == 0)
		{
			close(sockfd);
			player->stop = true;
		}
		else
		{
			if(errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
			{
				continue;
			}
			close(sockfd);
			player->stop = true;
		}
	}
	log_debug("rtsp_worker_start 线程退出, deviceid %d", deviceid);
	return NULL;
}

bool rtsp_request(t_video_play_info *player)
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
	player->reply_info = new t_rtsp_reply_info;
	memset(player->reply_info, 0, sizeof(t_rtsp_reply_info));
	memcpy(player->reply_info->session, player->rtsp_info->session, 64);
	memcpy(player->reply_info->ssrc[0], player->rtsp_info->ssrc[0], 16);
	memcpy(player->reply_info->ssrc[1], player->rtsp_info->ssrc[1], 16);
	return true;
}

bool send_rtsp_command(t_video_play_info *player, int type)
{
	char buffer[MAX_BUF_SIZE] = { 0 };
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

	// 发送command数据
	for(int i = 0; i < 3; i++)
	{
		int n = send(player->device_conn->sockfd, buffer, buflen, 0);
		if(n > 0)
		{
			break;
		}

		if((errno != EINTR && errno != EWOULDBLOCK && errno != EAGAIN))
		{
			return false;
		}
	}

	log_debug("***************** rtsp send message **************************\n"
			  "%s", buffer);
	log_debug("***************** rtsp send msg end **************************\n", buffer);

	// 接收reply数据
	memset(buffer, 0, MAX_BUF_SIZE);
	int length = 0;
	for(int j = 0; j < 3; j++)
	{
		int n = recv(player->device_conn->sockfd, buffer + length, MAX_BUF_SIZE - length, 0);
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
			return false;
		}
	}
	log_debug("***************** rtsp recv message **************************\n"
			  "%s", buffer);
	log_debug("***************** rtsp recv msg end **************************\n", buffer);

	// 处理应答数据
	return rtsp_reply_parse(player->rtsp_info, buffer, length, type);
}



bool rtsp_reply_parse(t_rtsp_info *info, char *buffer, int buflen, int cmd)
{
	int result = true;
	char *buf = new char[buflen - 4 + 1];
	memset(buf, 0, buflen - 4 + 1);
	memcpy(buf, buffer, buflen - 4);
	string message = buf;

	if(buf != NULL)
	{
		delete buf;
		buf = NULL;
	}

	int line_count = 0;
	string *lines = get_part_string(message, "\r\n", line_count);
	for(int i = 0; i < line_count; i++)
	{
		if(result == false)
		{
			break;
		}

		int count = 0;
		string *parts = get_part_string(lines[i], " ", count);
		
		// 根据发送指令解析数据
		switch(cmd)
		{
			case enum_cmd_options:
				break;
			case enum_cmd_describe:
			case enum_cmd_describe_secret:
				if(strcmp(parts[0].c_str(), "RTSP/1.0") == 0)
				{
					if(strcmp(parts[1].c_str(), "401") == 0)
					{
						info->secret = true;
					}
					else if(strcmp(parts[1].c_str(), "200") == 0)
					{
						result = true;
					}
					else
					{
						log_debug("rtsp reply parse failed, errno %s\n", parts[1].c_str());
						result = false;
					}
				}
				else if(strcmp(parts[0].c_str(), "WWW-Authenticate:") == 0)
				{
					if(strcmp(parts[1].c_str(), "Digest") == 0)
					{
						for(int j = 2; j < count; j++)
						{
							string_replace(parts[j], '\"');
							string_replace(parts[j], ',');
							int n = 0;
							string *args = get_part_string(parts[j], "=", n);
							if(n >= 2)
							{
								if(strcmp(args[0].c_str(), "realm") == 0)
								{
									memcpy(info->realm, args[1].c_str(), args[1].length());
								}
								else if(strcmp(args[0].c_str(), "nonce") == 0)
								{
									memcpy(info->nonce, args[1].c_str(), args[1].length());
								}
							}
							if(args != NULL)
							{
								delete [] args;
								args = NULL;
							}
						}
					}
				}
				else if(strcmp(parts[0].c_str(), "m=video") == 0)
				{
					for(; i < line_count; i++)
					{
						if(strncmp(lines[i].c_str(), "a=control:", 10) == 0)
						{
							string tmp = lines[i].substr(10, lines[i].length());
							memcpy(info->video_url, tmp.c_str(), tmp.length());
						}
						else if(strncmp(lines[i].c_str(), "m=audio", 7) == 0)
						{
							i--;
							break;
						}
					}
				}
				else if(strcmp(parts[0].c_str(), "m=audio") == 0)
				{
					for(; i < line_count; i++)
					{
						if(strncmp(lines[i].c_str(), "a=control:", 10) == 0)
						{
							string tmp = lines[i].substr(10, lines[i].length());
							memcpy(info->audio_url, tmp.c_str(), tmp.length());
						}
						else if(strncmp(lines[i].c_str(), "m=video", 7) == 0)
						{
							i--;
							break;
						}
					}
				}
				break;
			case enum_cmd_setup_video:
			case enum_cmd_setup_audio:
				if(strcmp(parts[0].c_str(), "Session:") == 0)
				{
					int n = 0;
					string *strs = get_part_string(parts[1], ";", n);
					memcpy(info->session, strs[0].c_str(), strs[0].length());
					if(strs != NULL)
					{
						delete [] strs;
						strs = NULL;
					}
				}
				else if(strcmp(parts[0].c_str(), "Transport:") == 0)
				{
					int n = 0;
					string *strs = get_part_string(parts[1], ";", n);
					for(int j = 2; j < n; j++)
					{
						string_replace(strs[j], '\"');
						string_replace(strs[j], ',');
						int m = 0;
						string *args = get_part_string(strs[j], "=", m);
						if(m >= 2)
						{
							if(strcmp(args[0].c_str(), "ssrc") == 0)
							{
								if(cmd == enum_cmd_setup_video)
								{
									memcpy(info->ssrc[0], args[1].c_str(), args[1].length());
								}
								else
								{
									memcpy(info->ssrc[1], args[1].c_str(), args[1].length());
								}
							}
						}
						if(args != NULL)
						{
							delete [] args;
							args = NULL;
						}
					}
					if(strs != NULL)
					{
						delete [] strs;
						strs = NULL;
					}
				}
				break;
			case enum_cmd_play:
				break;
			case enum_cmd_teardown:
				break;
			default:
				break;
		}
		if(parts != NULL)
		{
			delete [] parts;
			parts = NULL;
		}
	}
	if(lines != NULL)
	{
		delete [] lines;
		lines = NULL;
	}
	return result;
}

