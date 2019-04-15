// =====================================================================================
//	Copyright (C) 2019 by Jiaxing Shao.All rights reserved.
//	文 件 名:  http_handle.cpp
//	作    者:  Jiaxing Shao, 13315567369@163.com
//	版 本 号:  1.0
//	创建时间:  2019年04月10日 17时08分32秒
//	Compiler:  g++
//	描    述:  
// =====================================================================================

#include <iostream>
#include <memory>
using namespace std;
#include <time.h>
#include "rtsp_struct.h"
#include "rtsp_client.h"
#include "rtsp_server.h"
#include "rtsp_protocol.h"
#include "http_handle.h"
#include "jsoncpp/json/json.h"
#include "rtsp_task.h"
#include "logfile.h"

// 存储的设备信息
extern map<unsigned int, t_device_info*> g_mapDeviceInfo;
extern map<unsigned int, t_video_play_info*> g_mapVideoPlay;
extern char g_localhost[16];

mg_serve_http_opts HttpServer::s_server_option;
std::string HttpServer::s_web_dir = "./web";
std::unordered_map<std::string, ReqHandler> HttpServer::s_handler_map;

bool handle_describe(std::string url, std::string body, mg_connection *c, OnRspCallback rsp_callback)
{
	/*
	std::cout << "handle describe: " << std::endl;
	std::cout << "url: " << url << std::endl;
	std::cout << "body: " << body << std::endl;
    */

	string ret;

	do{
		Json::Reader reader;
		Json::Value root;
		if(!reader.parse(body.c_str(), root))
		{
			ret = "bad request";
			break;
		}
		int deviceid = root["deviceid"].asInt();
		int service_port = deviceid % 10000 + 10000;
		string rtsp_url;

		if(g_mapVideoPlay.find(deviceid) == g_mapVideoPlay.end())
		{	// 若不在当前播放列表内
			map<unsigned int, t_device_info*>::iterator iter = g_mapDeviceInfo.find(deviceid);
			if(iter != g_mapDeviceInfo.end())
			{	// 添加到播放列表
				video_task_add(deviceid);
				t_video_play_info *play_info = video_task_get(deviceid);
				if(play_info == NULL)
				{
					break;
				}

				// 设备基础信息
				play_info->device_info = new t_device_info;
				memcpy(play_info->device_info, iter->second, sizeof(t_device_info));

				// rtsp连接信息
				play_info->rtsp_info = new t_rtsp_info;
				memset(play_info->rtsp_info, 0, sizeof(t_rtsp_info));
				memcpy(play_info->rtsp_info->username, play_info->device_info->username, 32);
				memcpy(play_info->rtsp_info->password, play_info->device_info->password, 32);
				sprintf(play_info->rtsp_info->rtsp_url, "rtsp://%s:%d/h264/ch1/main/av_stream", 
						play_info->device_info->ipaddr, play_info->device_info->rtspport);

				// rtsp server,开启端口,并等待连接
				int sockfd = create_tcp_server(service_port);
				if(sockfd == -1)
				{
					// 释放内存
					video_task_remove(deviceid);
					video_play_free(play_info);
					ret = "create server failed";
					log_debug("端口号 %d 启动失败,端口号或已占用", play_info->rtsp_serv->port);
					break;
				}
				play_info->rtsp_serv = new tcp_server_info;
				memset(play_info->rtsp_serv, 0, sizeof(tcp_server_info));
				play_info->rtsp_serv->port = service_port;
				play_info->rtsp_serv->sockfd = sockfd;
				FD_ZERO(&play_info->serv_fds);
				FD_SET(sockfd, &play_info->serv_fds);
				pthread_mutex_init(&play_info->serv_lock, NULL);
				pthread_t pid_serv;
				play_info->thread_num += 1;
				pthread_create(&pid_serv, NULL, rtsp_server_start, (void*)&deviceid);

				// 与设备的连接信息
				play_info->device_conn = create_tcp_client_conn(play_info->device_info->ipaddr, play_info->device_info->rtspport);
				if(play_info->device_conn == NULL)
				{
					// 释放内存, 将stop置为true, 由启动的rtsp_server_start来处理内存释放
					play_info->stop = true;
					ret = "connect device failed";
					log_debug("连接到设备 %d 失败,ip[%s]", deviceid, play_info->device_info->ipaddr);
					break;
				}


				if(!rtsp_request(play_info))
				{
					// 释放内存, 将stop置为true, 由启动的rtsp_server_start来处理内存释放
					play_info->stop = true;
					ret = "rtsp protocol wrong";
					log_debug("与设备 %d rtsp对接失败,ip[%s]", deviceid, play_info->device_info->ipaddr);
					break;
				}
				else
				{
					log_debug("连接到设备 %d 成功,ip[%s],开始接收视频流", deviceid, play_info->device_info->ipaddr);
				}

				// 接收数据
				pthread_t pid_clnt;
				play_info->thread_num += 1;
				pthread_create(&pid_clnt, NULL, rtsp_worker_start, (void*)&deviceid);
				
				// 虚拟地址
				sprintf(play_info->reply_info->rtsp_url, "rtsp://%s:%d/video/h264/%d", g_localhost, service_port, deviceid);
				sprintf(play_info->reply_info->video_url, "%s/trackID=1", play_info->reply_info->rtsp_url);
				sprintf(play_info->reply_info->audio_url, "%s/trackID=2", play_info->reply_info->rtsp_url);
				rtsp_url = play_info->reply_info->rtsp_url;
				log_debug("生成虚拟rtsp地址: %s", rtsp_url.c_str());
			}
			else
			{// 此ID在设备列表内不存在
				ret = "no this device";
				break;
			}
		}
		else
		{
			rtsp_url = g_mapVideoPlay[deviceid]->reply_info->rtsp_url;
		}
		char response[256] = { 0 };
		sprintf(response, "{\"result\":false, \"message\":\"%s\"}", rtsp_url.c_str());
		rsp_callback(c, response);
		return true;
	}while(0);

	char response[256] = { 0 };
	sprintf(response, "{\"result\":false, \"message\":\"%s\"}", ret.c_str());
	rsp_callback(c, response);
	return true;
}

bool handle_undescribe(std::string url, std::string body, mg_connection *c, OnRspCallback rsp_callback)
{
	Json::Reader reader;
	Json::Value root;
	if(reader.parse(body.c_str(), root))
	{
		int deviceid = root["deviceid"].asInt();
		cout << deviceid << endl;
		rsp_callback(c, "{\"result\":\"true\"}");
	}
	else
	{
		rsp_callback(c, "{\"result\":\"false\"}");
	}
	return true;
}

