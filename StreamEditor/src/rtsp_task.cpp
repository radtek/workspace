// =====================================================================================
//	Copyright (C) 2019 by Jiaxing Shao.All rights reserved.
//	文 件 名:  rtsp_task.cpp
//	作    者:  Jiaxing Shao, 13315567369@163.com
//	版 本 号:  1.0
//	创建时间:  2019年04月11日 17时53分58秒
//	Compiler:  g++
//	描    述:  
// =====================================================================================

#include <map>
using namespace std;
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

extern map<unsigned int, t_device_video_play*> g_mapDeviceVideoPlay;
extern LOG_QUEUE *log_queue;
extern tcp_server_info *g_rtsp_serv;
extern int g_max_device_count;

bool get_device_info()
{
	do{
		char ora_ip[16] = { 0 };
		char ora_port[8] = { 0 };
		char ora_user[32] = { 0 };
		char ora_pwd[32] = { 0 };
		char ora_name[16] = { 0 };
		GetConfigureString("oracle.ipaddr", ora_ip, 16, "127.0.0.1", CONFFILE);
		GetConfigureString("oracle.port", ora_port, 8, "1521", CONFFILE);
		GetConfigureString("oracle.username", ora_user, 32, "EHL_VIPS", CONFFILE);
		GetConfigureString("oracle.password", ora_pwd, 32, "ehl1234", CONFFILE);
		GetConfigureString("oracle.name", ora_name, 16, "RACDB", CONFFILE);
		
		t_db_conn *conn = create_otl_conn_oracle(ora_user, ora_pwd, ora_name, ora_ip, ora_port);
		if(database_open(conn) == -1)
		{
			log_info(log_queue, "数据库连接失败, 程序退出, [%s]", conn->conn_str);
			break;
		}

		if(!select_device_info(conn))
		{
			log_info(log_queue, "获取设备信息失败, 程序退出");
			break;
		}
		else
		{
			log_debug("获取设备信息成功");
			map<unsigned int, t_device_video_play*>::iterator iter;
			for(iter = g_mapDeviceVideoPlay.begin(); iter != g_mapDeviceVideoPlay.end(); iter++)
			{
				log_debug("DeviceID: %10d, DeviceType: %.1d, IP: %15s, Username: %8s, Password: %12s",
						iter->second->device_info->deviceid, iter->second->device_info->devicetype, iter->second->device_info->ipaddr,
						iter->second->device_info->username, iter->second->device_info->password);

				log_info(log_queue, "DeviceID: %10d, DeviceType: %.1d, IP: %15s, Username: %8s, Password: %12s",
						iter->second->device_info->deviceid, iter->second->device_info->devicetype, iter->second->device_info->ipaddr,
						iter->second->device_info->username, iter->second->device_info->password);
			}
		}
		database_close(conn);
		return true;
	}while(0);
	return false;
}

bool select_device_info(t_db_conn *conn)
{
	string sql = "select videodeviceid, devicename, devicetype,deviceip,deviceport,reguser,regpwd from T_TVMS_DEVICE order by videodeviceid";
	try
	{
		otl_stream select(1, sql.c_str(), conn->conn);
		int count = 0;
		while(!select.eof())
		{
			if(count > g_max_device_count)
			{
				log_info(log_queue, "超过最大设备数限制, 取前 %d 个设备.", g_max_device_count);
				break;
			}

			t_device_video_play *player = (t_device_video_play*)malloc(sizeof(t_device_video_play));
			player->device_info = (t_device_info*)malloc(sizeof(t_device_info));
			memset(player->device_info, 0, sizeof(t_device_info));
			player->dev_rtsp_info = (t_rtsp_info*)malloc(sizeof(t_rtsp_info));
			memset(player->dev_rtsp_info, 0, sizeof(t_rtsp_info));
			player->vir_rtsp_info = (t_rtsp_info*)malloc(sizeof(t_rtsp_info));
			memset(player->vir_rtsp_info, 0, sizeof(t_rtsp_info));
			player->rtp_array = create_byte_array(1024 * 1024 * 16);
			pthread_mutex_init(&player->lock, NULL);
			player->stop = true;

			select >> player->device_info->deviceid;
			select >> player->device_info->crossname;
			select >> player->device_info->devicetype;
			select >> player->device_info->ipaddr;
			select >> player->device_info->port;
			select >> player->device_info->username;
			select >> player->device_info->password;
			player->device_info->rtspport = 554;
			g_mapDeviceVideoPlay[player->device_info->deviceid] = player;
			count += 1;
		}
		select.close();
	}
	catch(otl_exception &p)
	{
		return false;
	}
	return true;
}

void video_play_free(t_device_video_play* &player)
{
}

t_device_video_play *video_task_get(int deviceid)
{
	t_device_video_play *player = NULL;
	map<unsigned int, t_device_video_play*>::iterator iter;
	iter = g_mapDeviceVideoPlay.find(deviceid);
	if(iter != g_mapDeviceVideoPlay.end())
	{
		player = iter->second;
		return player;
	}
	return NULL;
}

int get_rtp_buffer(t_byte_array* &rtp_array, unsigned char *buf)
{
	int length = -1;
	if(get_byte_array(rtp_array, (char*)buf, 4) >= 0)
	{
		if(buf[0] != 0x24)
		{
			return -1;
		}

		length = buf[2] * 256 + buf[3];
		while(true)
		{
			if(get_byte_array(rtp_array, (char*)buf + 4, length) >= 0)
			{
				break;
			}
			sleep(0.01);
		}
		length += 4;
	}
	return length;
}

void *byte_array_process_start(void *arg)
{
	int deviceid = *((int*)arg);
	pthread_detach(pthread_self());
	t_device_video_play *player = video_task_get(deviceid);
	if(player == NULL)
	{
		log_debug("byte_array_process_start failed, deviceid %d", deviceid);
		log_info(log_queue, "视频流处理线程启动失败, 设备ID %d", deviceid);
		pthread_exit(NULL);
	}
	else
	{
		log_debug("byte_array_process_start success, deviceid %d", deviceid);
		log_info(log_queue, "视频流处理线程启动成功, 设备ID %d", deviceid);
	}
	unsigned char buffer[256 * 256];
	int length = 0;
	while(true)
	{
		if(player->stop)
		{
			break;
		}
		
		length = get_rtp_buffer(player->rtp_array, buffer);
		if(length == -1)
		{
			clean_byte_array(player->rtp_array);
			log_info(log_queue, "获取RTP流数据失败, 设备ID %d.", player->device_info->deviceid);
			continue;
		}

		if(g_rtsp_serv->device[player->serv_pos]->clnt_count != 0)
		{
			int clnt_count = g_rtsp_serv->device[player->serv_pos]->clnt_count;
			for(int i = 0; i < clnt_count; i++)
			{
				send(g_rtsp_serv->device[player->serv_pos]->clntfd[i], buffer, length, 0);
			}
		}
	}
	log_debug("byte_array_process_start 线程退出");
	log_info(log_queue, "视频流数据处理线程退出, 设备ID %d.", deviceid);
	return NULL;
}

void *rtsp_worker_start(void *arg)
{
	int deviceid = *((int*)arg);
	pthread_detach(pthread_self());
	t_device_video_play *player = video_task_get(deviceid);
	if(player == NULL)
	{
		log_info(log_queue, "设备视频流数据获取线程启动失败, 未知的设备ID %d.", deviceid);
		log_debug("rtsp_worker_start failed, deviceid %d", deviceid);
		pthread_exit(NULL);
	}
	else
	{
		log_info(log_queue, "设备视频流数据获取线程启动, 设备ID %d.", deviceid);
		log_debug("rtsp_worker_start success, deviceid %d", deviceid);
	}

	char buffer[MAX_VIDEO_CACHE] = { 0 };
	while(true)
	{
		if(player->stop)
		{
			break;
		}

		int n = recv(player->sockfd, buffer, MAX_VIDEO_CACHE, 0);
		if(n > 0)
		{
			put_byte_array(player->rtp_array, buffer, n);
		}
		else if(n == 0)
		{
			close(player->sockfd);
			g_rtsp_serv->device[player->serv_pos]->ready_stop = true;
		}
		else
		{
			if(errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
			{
				continue;
			}
			close(player->sockfd);
			g_rtsp_serv->device[player->serv_pos]->ready_stop = true;
		}
	}
	log_info(log_queue, "视频流数据获取线程退出, 设备ID %d.", deviceid);
	return NULL;
}

void *rtsp_server_start(void *arg)
{
	log_info(log_queue, "RTSP视频服务线程启动");
	pthread_detach(pthread_self());

	fd_set fds;
	int result = 0;
	int maxfd = 0;
	struct timeval timeout;
	char buffer[MAX_BUF_SIZE] = { 0 };

	while(true)
	{
		for(int i = 0; i < g_rtsp_serv->device_count; i++)
		{
			if(g_rtsp_serv->device[i]->clnt_count == 0 && !g_rtsp_serv->device[i]->stop)
			{
				g_rtsp_serv->device[i]->time_count += 5;
				if(g_rtsp_serv->device[i]->time_count >= MAX_SERVICE_WAIT_TIME)
				{
					g_rtsp_serv->device[i]->ready_stop = true;
					log_info(log_queue, "设备ID%d, %d秒无连接, 停止任务", g_rtsp_serv->device[i]->deviceid, MAX_SERVICE_WAIT_TIME);
				}
			}
			
			if(g_rtsp_serv->device[i]->ready_stop)
			{
				// RTSP服务器停止提供服务
				g_rtsp_serv->device[i]->stop = true;
				int deviceid = g_rtsp_serv->device[i]->deviceid;
				t_device_video_play *player = video_task_get(deviceid);
				// 数据处理任务停止
				player->rtp_array->stop = true;
				// 内存释放任务添加到线程池来处理
				t_threadpool_task *task = create_threadpool_task();
				task->callback = task_release;
				task->arg = &g_rtsp_serv->device[i]->deviceid;
				threadpool_add_task(task);
				g_rtsp_serv->device[i]->ready_stop = false;
			}
		}
		
		maxfd = g_rtsp_serv->maxfd;
		timeout.tv_sec = 5;
		timeout.tv_usec = 0;
		fds = g_rtsp_serv->fds;
		result = select(maxfd + 1, &fds, NULL, NULL, &timeout);
		if(result > 0)
		{
			if(FD_ISSET(g_rtsp_serv->sockfd, &fds))
			{
				struct sockaddr_in clnt_addr;
				memset(&clnt_addr, 0, sizeof(clnt_addr));
				socklen_t clnt_addr_size = sizeof(clnt_addr);
				int sockfd = accept(g_rtsp_serv->sockfd, (sockaddr*)&clnt_addr, &clnt_addr_size);
				if(sockfd > 0)
				{
					tcp_client_info *clnt = (tcp_client_info*)malloc(sizeof(tcp_client_info));
					memset(clnt, 0, sizeof(tcp_client_info));
					memcpy(clnt->ipaddr, inet_ntoa(clnt_addr.sin_addr), 16);
					clnt->sockfd = sockfd;
					t_threadpool_task *task = create_threadpool_task();
					task->callback = &rtsp_response;
					task->arg = (void *)clnt;
					task->release = true;
					threadpool_add_task(task);
				}
				result -= 1;
			}

			for(int i = 0; i < g_rtsp_serv->device_count; i++)
			{
				if(result == 0)
				{
					continue;
				}

				if(!g_rtsp_serv->device[i]->stop)
				{
					for(int j = 0; j < g_rtsp_serv->device[i]->clnt_count; j++)
					{
						if(FD_ISSET(g_rtsp_serv->device[i]->clntfd[j], &fds))
						{
							int n = recv(g_rtsp_serv->device[i]->clntfd[j], buffer, MAX_BUF_SIZE, 0);
							if(n <= 0)
							{
								pthread_mutex_lock(&g_rtsp_serv->lock);
								int count = g_rtsp_serv->device[i]->clnt_count;
								FD_CLR(g_rtsp_serv->device[i]->clntfd[j], &g_rtsp_serv->fds);
								close(g_rtsp_serv->device[i]->clntfd[j]);
								if(j < count - 1)
								{
									g_rtsp_serv->device[i]->clntfd[j] = g_rtsp_serv->device[i]->clntfd[count - 1];
								}
								g_rtsp_serv->device[i]->clnt_count -= 1;
								log_info(log_queue, "客户端主动断开连接 %d, 当前连接数%d.", g_rtsp_serv->device[i]->deviceid, g_rtsp_serv->device[i]->clnt_count);
								log_debug("客户端主动断开连接 %d, 当前连接数%d.", g_rtsp_serv->device[i]->deviceid, g_rtsp_serv->device[i]->clnt_count);
								pthread_mutex_unlock(&g_rtsp_serv->lock);
								j -= 1;
							}
						}
					}
				}
			}
		}
		else
		{
		}
	}
	log_info(log_queue, "RTSP视频服务线程退出");
	return NULL;
}

void task_release(void *arg)
{
	int deviceid = *((int*)arg);
	t_device_video_play *player = video_task_get(deviceid);
	player->rtp_array->stop = true;
	player->stop = true;
	for(int i = 0; i < 2; i++)
	{
		pthread_join(player->pid[i], NULL);
	}

	pthread_mutex_lock(&g_rtsp_serv->lock);
	g_rtsp_serv->device[player->serv_pos]->stop = true;
	for(int i = 0; i < g_rtsp_serv->device[player->serv_pos]->clnt_count; i++)
	{
		FD_CLR(g_rtsp_serv->device[player->serv_pos]->clntfd[i], &g_rtsp_serv->fds);
		close(g_rtsp_serv->device[player->serv_pos]->clntfd[i]);
	}
	g_rtsp_serv->device[player->serv_pos]->clnt_count = 0;
	pthread_mutex_unlock(&g_rtsp_serv->lock);
}
