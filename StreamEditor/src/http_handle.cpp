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
#include <map>
using namespace std;
#include <time.h>
#include "functions.h"
#include "rtsp_struct.h"
#include "rtsp_client.h"
#include "rtsp_server.h"
#include "rtsp_protocol.h"
#include "http_handle.h"
#include "jsoncpp/json/json.h"
#include "rtsp_task.h"
#include "logfile.h"
#include "bytearray.h"

// 存储的设备信息

// 直接向线程传入int地址时,会存在线程内未使用时,变量即被释放的情况

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
		string rtsp_url;

		t_device_video_play *player = video_task_get(deviceid);
		if(player == NULL)
		{
			ret = "no this device";
			break;
		}
		else
		{
			pthread_mutex_lock(&player->lock);
			if(player->stop)
			{
				player->sockfd = connect_server(player->device_info->ipaddr, player->device_info->rtspport);
				if(!rtsp_request(player))
				{
					pthread_mutex_unlock(&player->lock);
					ret = "rtsp protocol wrong";
					log_debug("与设备 %d rtsp对接失败,ip[%s]", deviceid, player->device_info->ipaddr);
					break;
				}
				player->stop = false;
				log_debug("连接到设备 %d 成功,ip[%s],开始接收视频流", deviceid, player->device_info->ipaddr);
				pthread_create(&player->pid[0], NULL, rtsp_worker_start, (void*)&player->device_info->deviceid);
				pthread_create(&player->pid[1], NULL, byte_array_process_start, (void*)&player->device_info->deviceid);
				pthread_mutex_unlock(&player->lock);
				rtsp_url = player->vir_rtsp_info->rtsp_url;
				log_debug("返回虚拟rtsp地址: %s", rtsp_url.c_str());
			}
			else
			{
				rtsp_url = player->vir_rtsp_info->rtsp_url;
				pthread_mutex_lock(&player->lock);
				log_debug("返回虚拟rtsp地址: %s", rtsp_url.c_str());
			}
		}
		char response[256] = { 0 };
		sprintf(response, "{\"result\":true, \"message\":\"%s\"}", rtsp_url.c_str());
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

