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

// 存储的设备信息
extern map<unsigned int, t_device_info*> g_mapDeviceInfo;
extern map<unsigned int, t_video_play_info*> g_mapVideoPlay;
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
	t_video_play_info *play_info = new t_video_play_info;
	play_info->device_info = NULL;
	play_info->device_conn = NULL;
	play_info->rtsp_serv = NULL;
	play_info->rtsp_info = NULL;

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

		if(g_mapVideoPlay.find(deviceid) == g_mapVideoPlay.end())
		{	// 若不在当前播放列表内
			map<unsigned int, t_device_info*>::iterator iter = g_mapDeviceInfo.find(deviceid);
			if(iter != g_mapDeviceInfo.end())
			{	// 添加到播放列表

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

				// rtsp server
				int sockfd = create_tcp_server(service_port);
				if(sockfd == -1)
				{
					ret = "create server failed";
					break;
				}
				play_info->rtsp_serv = new tcp_server_info;
				memset(play_info->rtsp_serv, 0, sizeof(tcp_server_info));
				play_info->rtsp_serv->port = service_port;
				play_info->rtsp_serv->sockfd = sockfd;
				play_info->rtsp_serv->last_time = time(NULL);

				// 与设备的连接信息
				play_info->device_conn = create_tcp_client_conn(play_info->device_info->ipaddr, play_info->device_info->rtspport);
				if(play_info->device_conn == NULL)
				{
					ret = "connect device failed";
					break;
				}

				if(!rtsp_request(play_info))
				{
					ret = "rtsp protocol wrong";
					break;
				}
			}
			else
			{// 此ID在设备列表内不存在
				ret = "no this device";
				break;
			}
		}
		else
		{
		}

		char response[256] = { 0 };
		sprintf(response, "{\"result\":true,\"message\":\"rtsp://192.168.136.120:%d/video/h264/%d\"}", service_port, deviceid);
		rsp_callback(c, response);
		return true;
	}while(0);

	if(play_info->rtsp_serv != NULL)
	{
		delete play_info->rtsp_serv;
		play_info->rtsp_serv = NULL;
	}
	if(play_info->device_info != NULL)
	{
		delete play_info->device_info;
		play_info->device_info = NULL;
	}
	if(play_info->rtsp_info != NULL)
	{
		delete play_info->rtsp_info;
		play_info->rtsp_info = NULL;
	}
	if(play_info->device_conn != NULL)
	{
		delete play_info->device_conn;
		play_info->device_conn = NULL;
	}
	if(play_info != NULL)
	{
		delete play_info;
		play_info = NULL;
	}

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

