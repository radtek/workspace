// =====================================================================================
//	Copyright (C) 2019 by Jiaxing Shao.All rights reserved.
//	文 件 名:  main.cpp
//	作    者:  Jiaxing Shao, 13315567369@163.com
//	版 本 号:  1.0
//	创建时间:  2019年04月02日 11时33分58秒
//	Compiler:  g++
//	描    述:  
// =====================================================================================

#include <map>
#include <memory>
using namespace std;
#include <signal.h>

#include "threadpool.h"
#include "functions.h"
#include "logfile.h"
#include "otlcontrol.h"
#include "rtsp_server.h"
#include "rtsp_task.h"
#include "http_handle.h"
#include "websocket_server.h"

#define THREAD_NUM (20)
#define THREAD_TASK_NUM (1024)

map<unsigned int, t_device_video_play*> g_mapDeviceVideoPlay;

LOG_QUEUE *log_queue = NULL;
tcp_server_info *g_rtsp_serv;
int g_max_device_count;
WebSocketServer g_ws_serv;

void signal_handler(int signum);
void *thread_http(void *arg);

int main(int argc, char *argv[])
{
	struct sigaction sa;
	sa.sa_handler = signal_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(SIGABRT, &sa, NULL);
	sigaction(SIGFPE, &sa, NULL);
	sigaction(SIGSEGV, &sa, NULL);
	sigaction(SIGPIPE, &sa, NULL);


	char localhost[16] = { 0 };
	char networkhost[16] = { 0 };
	char http_port[8] = { 0 };
	char rtsp_port[8] = { 0 };
	char ws_port[8] = { 0 };
	char dev_count[8] = { 0 };
	char channel_type[8] = { 0 };
	GetConfigureString("local.ipaddr", localhost, 16, "127.0.0.1", CONFFILE);
	GetConfigureString("network.ipaddr", networkhost, 16, "127.0.0.1", CONFFILE);
	GetConfigureString("http.service.port", http_port, 8, "8000", CONFFILE);
	GetConfigureString("rtsp.service.port", rtsp_port, 8, "8001", CONFFILE);
	GetConfigureString("ws.service.port", ws_port, 8, "8002", CONFFILE);
	GetConfigureString("rtsp.device.count", dev_count, 8, "20", CONFFILE);
	GetConfigureString("rtsp.device.channel", channel_type, 8, "sub", CONFFILE);
	g_max_device_count = atoi(dev_count);

	// 启动日志线程
	start_log_thread();
	log_queue = create_log_queue("rtsp_logs");

	// 获取设备信息
	if(!get_device_info())
	{
		log_debug("获取设备信息失败.");
		return EXIT_FAILURE;
	}

	// 启动rtsp服务
	g_rtsp_serv = create_tcp_server(localhost, networkhost, rtsp_port);
	if(g_rtsp_serv == NULL)
	{
		log_debug("rtsp服务端口 %s 启动失败", rtsp_port);
		return EXIT_FAILURE;
	}
	else
	{
		log_debug("rtsp server port[%s] start success.", rtsp_port);
		log_info(log_queue, "RTSP服务端口开启成功，端口号 %s.", rtsp_port);
		// 设备最大限制数量
		int device_count = (g_mapDeviceVideoPlay.size() >= g_max_device_count) ? g_max_device_count : g_mapDeviceVideoPlay.size();
		map<unsigned int, t_device_video_play*>::iterator iter = g_mapDeviceVideoPlay.begin();
		for(int i = 0; i < device_count; i++)
		{
			g_rtsp_serv->device[i]->deviceid = iter->second->device_info->deviceid;
			iter->second->serv_pos = i;
			sprintf(iter->second->vir_rtsp_info->rtsp_url, "rtsp://%s:%d/video/h264/%d", 
					g_rtsp_serv->ipaddr, g_rtsp_serv->port, iter->second->device_info->deviceid);
			sprintf(iter->second->vir_rtsp_info->video_url, "%s/trackID=1", iter->second->vir_rtsp_info->rtsp_url);
			sprintf(iter->second->vir_rtsp_info->audio_url, "%s/trackID=2", iter->second->vir_rtsp_info->rtsp_url);
			if(strcmp(channel_type, "main") == 0)
			{
				// 主码流
				sprintf(iter->second->dev_rtsp_info->rtsp_url, "rtsp://%s:%d/h264/ch1/main/av_stream", 
						iter->second->device_info->ipaddr, iter->second->device_info->rtspport);
			}
			else
			{
				// 子码流
				sprintf(iter->second->dev_rtsp_info->rtsp_url, "rtsp://%s:%d/h264/ch1/sub/av_stream", 
						iter->second->device_info->ipaddr, iter->second->device_info->rtspport);
			}
			memcpy(iter->second->dev_rtsp_info->username, iter->second->device_info->username, 32);
			memcpy(iter->second->dev_rtsp_info->password, iter->second->device_info->password, 32);
			iter++;
		}
		g_rtsp_serv->device_count = device_count;
	}
	pthread_t pid;
	pthread_create(&pid, NULL, rtsp_server_start, NULL);

	// 开启线程池
	threadpool_start(THREAD_NUM, THREAD_TASK_NUM);
	log_debug("线程池启动完成, 线程数 %d, 最大任务数 %d", THREAD_NUM, THREAD_TASK_NUM);

	pthread_t tid;
	pthread_create(&tid, NULL, thread_http, (void*)http_port);

	// 开启websocket
	log_debug("websocket port[%s] start", ws_port);
	int port = atoi(ws_port);
	g_ws_serv.start(port);

	return EXIT_SUCCESS;
}

void signal_handler(int signum)
{
	if(signum == 1)
	{
		log_debug("SIGHUP");
	}
	else if(signum == 2)
	{
		log_debug("SIGINT");
	}
	else if(signum == 3)
	{
		log_debug("SIGQUIT");
	}
	else if(signum == 6)
	{
		log_debug("SIGABRT");
	}
	else if(signum == 8)
	{
		log_debug("SIGFPE");
	}
	else if(signum == 11)
	{
		log_debug("SIGSEGV");
	}
	else if(signum == 13)
	{
		log_debug("SIGPIPE");
	}
}

void *thread_http(void *arg)
{
	char* port = (char*)arg;
	log_info(log_queue, "HTTP服务端口开启，端口号 %s.", port);
	// 开启http端口
	std::string httpport = port;
	auto http_server = std::shared_ptr<HttpServer>(new HttpServer);
	http_server->Init(httpport);
	http_server->AddHandler("/rtsp/describe", handle_describe);
	http_server->AddHandler("/rtsp/undescribe", handle_undescribe);
	http_server->Start();
}

