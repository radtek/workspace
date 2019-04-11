// =====================================================================================
//	Copyright (C) 2019 by Jiaxing Shao.All rights reserved.
//	文 件 名:  rtsp_server.cpp
//	作    者:  Jiaxing Shao, 13315567369@163.com
//	版 本 号:  1.0
//	创建时间:  2019年04月10日 10时13分41秒
//	Compiler:  g++
//	描    述:  
// =====================================================================================

#include "rtsp_struct.h"
#include "rtsp_server.h"
#include "logfile.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
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

static void *tcp_server_worker(void *arg)
{
	t_video_play_info *player = (t_video_play_info*)arg;
	
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(player->rtsp_serv->sockfd, &fds);

	int maxfd = player->rtsp_serv->sockfd;
	fd_set tmp_fds;
	struct timeval timeout;
	timeout.tv_sec = 5;
	timeout.tv_usec = 0;
	int result = 0;

	while(true)
	{
		if(player->rtsp_serv->stop)
		{
			FD_CLR(&fds);
			close(player->rtsp_serv->sockfd);
			break;
		}

		tmp_fds = fds;
		result = select(maxfd + 1, &tmp_fds, NULL, NULL, &timeout);
		if(result > 0)
		{
			if(FD_ISSET(player->rtsp_serv->sockfd))
			{
				int sockfd = accept(player->rtsp_serv)
			}
			if(result == 0)
			{
				continue;
			}

			for(int i = 0; i < MAX_CLNT_ONLINE; i++)
			{
				if(player->rtsp_serv->clnt[i] != NULL)
				{
					if(FD_ISSET(player->rtsp_serv->clnt[i]->sockfd))
					{
					}
				}
			}
		}
	}
	cout << "tcp_server_worker 线程退出" << endl;
}

void tcp_server_stop(t_video_play_info *player)
{
	player->rtsp_serv->
}

void on_accept(int fd, short events, void *arg)
{
	struct event_base *base = (struct event_base*)arg;
	struct sockaddr_in clnt_addr;
	socklen_t clnt_addr_sz = sizeof(clnt_addr);
	memset(&clnt_addr, 0, clnt_addr_sz);
	int sock = accept(fd, (struct sockaddr*)&clnt_addr, &clnt_addr_sz);
	if(sock == -1)
	{
		return;
	}

	if(!recv_rtsp_command(sock))
	{
		log_info(log_queue, "recv_rtsp_command() error!");
		return;
	}

	tcp_conn_info *clnt = new tcp_conn_info;
	memset(clnt, 0, sizeof(tcp_conn_info));
	clnt->sockfd = sock;
	strncpy(clnt->ipaddr, inet_ntoa(clnt_addr.sin_addr), 16);

	printf("Client connected ip: %s\n", clnt->ipaddr);
}

bool recv_rtsp_command(int sock)
{
	return false;
}

