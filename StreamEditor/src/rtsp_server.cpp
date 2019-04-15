// =====================================================================================
//	Copyright (C) 2019 by Jiaxing Shao.All rights reserved.
//	文 件 名:  rtsp_server.cpp
//	作    者:  Jiaxing Shao, 13315567369@163.com
//	版 本 号:  1.0
//	创建时间:  2019年04月10日 10时13分41秒
//	Compiler:  g++
//	描    述:  
// =====================================================================================

#include "rtsp_task.h"
#include "rtsp_server.h"
#include "rtsp_protocol.h"
#include "logfile.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <pthread.h>
#include <errno.h>

extern LOG_QUEUE *log_queue;

int create_tcp_server(int port)
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

void *rtsp_server_start(void *arg)
{
	log_debug("rtsp_server_start 线程启动");
	pthread_detach(pthread_self());
	int deviceid = *((int*)arg);
	t_video_play_info *player = video_task_get(deviceid);

	int maxfd = player->rtsp_serv->sockfd;
	fd_set fds;
	struct timeval timeout;
	int result = 0;

	char buffer[MAX_BUF_SIZE] = { 0 };

	while(true)
	{
		if(player->stop)
		{
			log_debug("rtsp_server_start 线程准备退出");
			video_task_remove(player->device_info->deviceid);
			break;
		}

		if(player->rtsp_serv->clnt_count == 0)
		{
			player->rtsp_serv->time_count += 5;
			if(player->rtsp_serv->time_count >= 300)
			{
				player->stop = true;
				log_info(log_queue, "%s 长时间无连接, 停止任务", player->rtsp_info->rtsp_url);
				log_debug("%s 长时间无连接,停止任务", player->rtsp_info->rtsp_url);
				break;
			}
		}

		timeout.tv_sec = 5;
		timeout.tv_usec = 0;
		fds = player->serv_fds;
		result = select(maxfd + 1, &fds, NULL, NULL, &timeout);
		if(result > 0)
		{
			if(FD_ISSET(player->rtsp_serv->sockfd, &fds))
			{
				struct sockaddr_in clnt_addr;
				memset(&clnt_addr, 0, sizeof(clnt_addr));
				socklen_t clnt_addr_size = sizeof(clnt_addr);
				int sockfd = accept(player->rtsp_serv->sockfd, (sockaddr*)&clnt_addr, &clnt_addr_size);
				if(sockfd > 0)
				{
					if(player->rtsp_serv->clnt_count < MAX_CLNT_ONLINE)
					{
						do{
							// rtsp握手
							if(!rtsp_response(player, sockfd))
							{
								log_debug("%s rtsp协议对接失败", inet_ntoa(clnt_addr.sin_addr));
								close(sockfd);
								break;
							}

							// 握手成功后,添加到连接列表
							tcp_conn_info *conn = new tcp_conn_info;
							memset(conn, 0, sizeof(tcp_conn_info));
							conn->sockfd = sockfd;
							if(maxfd < sockfd)
							{
								maxfd = sockfd;
							}
							memcpy(conn->ipaddr, inet_ntoa(clnt_addr.sin_addr), 16);
							FD_SET(sockfd, &player->serv_fds);
							for(int i = 0; i < MAX_CLNT_ONLINE; i++)
							{
								if(player->rtsp_serv->clnt[i] == NULL)
								{
									player->rtsp_serv->clnt[i] = conn;
									pthread_mutex_lock(&player->serv_lock);
									player->rtsp_serv->clnt_count += 1;
									pthread_mutex_unlock(&player->serv_lock);
									break;
								}
							}
							// 计时重置
							player->rtsp_serv->time_count = 0;
							log_debug("%s 连接到设备视频服务, 设备编号 %d, 当前连接数 %d", conn->ipaddr, 
									player->device_info->deviceid, player->rtsp_serv->clnt_count);
						}while(0);
					}
					else
					{
						// 拒绝连接请求
						close(sockfd);
					}
				}
				result -= 1;
			}

			for(int i = 0; i < MAX_CLNT_ONLINE; i++)
			{
				if(result == 0)
				{
					continue;
				}
				if(player->rtsp_serv->clnt[i] != NULL)
				{
					if(FD_ISSET(player->rtsp_serv->clnt[i]->sockfd, &fds))
					{
						int n = recv(player->rtsp_serv->clnt[i]->sockfd, buffer, MAX_BUF_SIZE, 0);
						if(n <= 0)
						{
							// 移除连接
							tcp_conn_info *tmp = player->rtsp_serv->clnt[i];
							player->rtsp_serv->clnt[i] = NULL;
							if(tmp != NULL)
							{
								pthread_mutex_lock(&player->serv_lock);
								player->rtsp_serv->clnt_count -= 1;
								pthread_mutex_unlock(&player->serv_lock);
								log_debug("客户端断开连接, ip[%s], 设备编号 %d, 当前连接数 %d", tmp->ipaddr, 
										player->device_info->deviceid, player->rtsp_serv->clnt_count);
								FD_CLR(tmp->sockfd, &player->serv_fds);
								close(tmp->sockfd);
								delete tmp;
								tmp = NULL;
							}
						}
						else if(n > 0)
						{
							// do nothing
						}
					}
				}
			}
		}
	}
	// 等待10秒钟后,结束线程
	sleep(10);
	close(player->rtsp_serv->sockfd);
	video_play_free(player);
	log_debug("rtsp_server_start 线程退出");
	return NULL;
}

bool rtsp_response(t_video_play_info *player, int sockfd)
{
	while(true)
	{
		int n = recv_rtsp_command(player, sockfd);
		if(n == -1)
		{
			log_info(log_queue, "recv_rtsp_command %d failed.", n);
			return false;
		}
		else if(n == 4)
		{
			log_info(log_queue, "rtsp process done.");
			break;
		}
		else if(n == 5)
		{
			return false;
		}
	}
	return true;
}

int recv_rtsp_command(t_video_play_info *player, int sockfd)
{
	char buffer[MAX_BUF_SIZE] = { 0 };
	int buflen = 0;

	// 接收cmd数据
	memset(buffer, 0, MAX_BUF_SIZE);
	for(int j = 0; j < 3; j++)
	{
		buflen = recv(sockfd, buffer, MAX_BUF_SIZE, 0);
		if(buflen > 0)
		{
			break;
		}

		if((errno != EINTR && errno != EWOULDBLOCK && errno != EAGAIN))
		{
			return -1;
		}
	}

	// 处理应答数据
	int type = rtsp_cmd_parse(player->reply_info, buffer, buflen);

	memset(buffer, 0, MAX_BUF_SIZE);
	switch(type)
	{
		case 0:
			buflen = rtsp_reply_options(player->reply_info, buffer);
			break;
		case 1:
			buflen = rtsp_reply_describe(player->reply_info, buffer);
			break;
		case 2:
			buflen = rtsp_reply_setup(player->reply_info, buffer, 1);
			break;
		case 3:
			buflen = rtsp_reply_setup(player->reply_info, buffer, 2);
			break;
		case 4:
			buflen = rtsp_reply_play(player->reply_info, buffer);
			break;
		case 5:
			buflen = rtsp_reply_teardown(player->reply_info, buffer);
			break;
		default:
			break;
	}

	int len = 0;
	// 发送command数据
	for(int i = 0; i < 3; i++)
	{
		len = send(sockfd, buffer, buflen, 0);
		if(len > 0)
		{
			break;
		}

		if((errno != EINTR && errno != EWOULDBLOCK && errno != EAGAIN))
		{
			return -1;
		}
	}
	return type;
}

int rtsp_cmd_parse(t_rtsp_reply_info *info, char *buffer, int buflen)
{
	char *buf = new char[buflen - 4 + 1];
	memset(buf, 0, buflen - 4 + 1);
	memcpy(buf, buffer, buflen - 4);
	string message = buf;
	int type = 0;

	if(buf != NULL)
	{
		delete buf;
		buf = NULL;
	}

	int lines_count = 0;
	string *lines = get_part_string(message, "\r\n", lines_count);
	int head_count = 0;
	string *header = get_part_string(lines[0], " ", head_count);
		
	if(strcmp(header[0].c_str(), "OPTIONS") == 0)
	{
		type = 0;
		for(int i = 1; i < lines_count; i++)
		{
			int value_count = 0;
			string *value_data = get_part_string(lines[i], " ", value_count);
			if(strcmp(value_data[0].c_str(), "CSeq:") == 0)
			{
				info->cmd_seq = atoi(value_data[1].c_str());
			}
			if(value_data != NULL)
			{
				delete [] value_data;
				value_data = NULL;
			}
		}
	}
	else if(strcmp(header[0].c_str(), "DESCRIBE") == 0)
	{
		type = 1;
		for(int i = 1; i < lines_count; i++)
		{
			int value_count = 0;
			string *value_data = get_part_string(lines[i], " ", value_count);
			if(strcmp(value_data[0].c_str(), "CSeq:") == 0)
			{
				info->cmd_seq = atoi(value_data[1].c_str());
			}
			if(value_data != NULL)
			{
				delete [] value_data;
				value_data = NULL;
			}
		}
	}
	else if(strcmp(header[0].c_str(), "SETUP") == 0)
	{
		if(strcmp(header[1].c_str(), info->video_url) == 0)
		{
			type = 2;
		}
		else
		{
			type = 3;
		}

		for(int i = 1; i < lines_count; i++)
		{
			int value_count = 0;
			string *value_data = get_part_string(lines[i], " ", value_count);
			if(strcmp(value_data[0].c_str(), "CSeq:") == 0)
			{
				info->cmd_seq = atoi(value_data[1].c_str());
			}
			else if(strcmp(value_data[0].c_str(), "Transport:") == 0)
			{
				memcpy(info->transport, value_data[1].c_str(), value_data[1].length());
			}

			if(value_data != NULL)
			{
				delete [] value_data;
				value_data = NULL;
			}
		}
	}
	else if(strcmp(header[0].c_str(), "PLAY") == 0)
	{
		type = 4;
		for(int i = 1; i < lines_count; i++)
		{
			int value_count = 0;
			string *value_data = get_part_string(lines[i], " ", value_count);
			if(strcmp(value_data[0].c_str(), "CSeq:") == 0)
			{
				info->cmd_seq = atoi(value_data[1].c_str());
			}
			if(value_data != NULL)
			{
				delete [] value_data;
				value_data = NULL;
			}
		}
	}
	else if(strcmp(header[0].c_str(), "TEARDOWN") == 0)
	{
		type = 5;
		for(int i = 1; i < lines_count; i++)
		{
			int value_count = 0;
			string *value_data = get_part_string(lines[i], " ", value_count);
			if(strcmp(value_data[0].c_str(), "CSeq:") == 0)
			{
				info->cmd_seq = atoi(value_data[1].c_str());
			}
			if(value_data != NULL)
			{
				delete [] value_data;
				value_data = NULL;
			}
		}
	}
	
	if(header != NULL)
	{
		delete [] header;
		header = NULL;
	}
	if(lines != NULL)
	{
		delete [] lines;
		lines = NULL;
	}
	return type;
}

