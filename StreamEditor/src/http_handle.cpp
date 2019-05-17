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
#include "threadpool.h"
#include <jsoncpp/json/json.h>
#include "rtsp_task.h"
#include "logfile.h"
#include "bytearray.h"

// 直接向线程传入int地址时,会存在线程内未使用时,变量即被释放的情况

extern LOG_QUEUE *log_queue;
extern tcp_server_info *g_rtsp_serv;
mg_serve_http_opts HttpServer::s_server_option;
std::string HttpServer::s_web_dir = "./web";
std::unordered_map<std::string, ReqHandler> HttpServer::s_handler_map;

void get_video_stream(void *arg)
{
	t_device_video_play *player = (t_device_video_play*)arg;

	pthread_mutex_lock(&player->lock);
	player->sockfd = connect_server(player->device_info->ipaddr, player->device_info->rtspport);
	if(player->sockfd == -1)
	{
		player->stop = true;
		pthread_mutex_unlock(&player->lock);
		log_debug("连接设备 %d 失败, ip[%s], 请检查设备网络状况.", player->device_info->deviceid, player->device_info->ipaddr);
		log_info(log_queue, "连接设备 %d 失败, ip[%s], 请检查设备网络状况.", player->device_info->deviceid, player->device_info->ipaddr);
		return;
	}

	if(!rtsp_request(player))
	{
		player->stop = true;
		pthread_mutex_unlock(&player->lock);
		log_debug("与设备 %d rtsp对接失败,ip[%s]", player->device_info->deviceid, player->device_info->ipaddr);
		log_info(log_queue, "与设备 %d rtsp对接失败,ip[%s]", player->device_info->deviceid, player->device_info->ipaddr);
		return;
	}

	// RTSP服务提供标志
	player->stop = false;
	start_byte_array(player->rtp_array);
	g_rtsp_serv->device[player->serv_pos]->stop = false;
	g_rtsp_serv->device[player->serv_pos]->time_count = 0;
	log_info(log_queue, "连接到设备 %d 成功,ip[%s],开始接收视频流", player->device_info->deviceid, player->device_info->ipaddr);
	pthread_create(&player->pid[0], NULL, rtsp_worker_start, (void*)&player->device_info->deviceid);
	pthread_create(&player->pid[1], NULL, byte_array_process_start, (void*)&player->device_info->deviceid);
	pthread_mutex_unlock(&player->lock);
}

bool handle_describe(std::string url, std::string body, mg_connection *c, OnRspCallback rsp_callback)
{
	log_debug("接收到视频订阅请求, %s", body.c_str());
	string ret;
	do{
		Json::Reader reader;
		Json::Value root;
		if(!reader.parse(body.c_str(), root))
		{
			ret = "bad request";
			log_info(log_queue, "Http 错误的消息类型, %s", body.c_str());
			break;
		}

		int deviceid = root["deviceid"].asInt();
		string rtsp_url;

		t_device_video_play *player = video_task_get(deviceid);
		if(player == NULL)
		{
			ret = "no this device";
			log_info(log_queue, "不存在此设备ID, %d", deviceid);
			break;
		}
		else
		{

			pthread_mutex_lock(&player->lock);
			if(player->stop)
			{
				t_threadpool_task *task = create_threadpool_task();
				task->callback = &get_video_stream;
				task->arg = (void *)player;
				threadpool_add_task(task);
				pthread_mutex_unlock(&player->lock);

				/*
				player->sockfd = connect_server(player->device_info->ipaddr, player->device_info->rtspport);
				if(player->sockfd == -1)
				{
					pthread_mutex_unlock(&player->lock);
					ret = "can't connect to device.";
					log_info(log_queue, "连接设备 %d 失败, ip[%s], 请检查设备网络状况.", deviceid, player->device_info->ipaddr);
					break;
				}
				if(!rtsp_request(player))
				{
					pthread_mutex_unlock(&player->lock);
					ret = "rtsp protocol wrong";
					log_info(log_queue, "与设备 %d rtsp对接失败,ip[%s]", deviceid, player->device_info->ipaddr);
					break;
				}
				// RTSP服务提供标志
				player->stop = false;
				start_byte_array(player->rtp_array);
				g_rtsp_serv->device[player->serv_pos]->stop = false;
				g_rtsp_serv->device[player->serv_pos]->time_count = 0;
				log_info(log_queue, "连接到设备 %d 成功,ip[%s],开始接收视频流", deviceid, player->device_info->ipaddr);
				pthread_create(&player->pid[0], NULL, rtsp_worker_start, (void*)&player->device_info->deviceid);
				pthread_create(&player->pid[1], NULL, byte_array_process_start, (void*)&player->device_info->deviceid);
				*/

				bool bRet = false;
				int i = 0;
				do{
					bRet = player->stop;
					if(!bRet)
					{
						break;
					}
					sleep(1);
				}while(i <= 10);

				if(!bRet)
				{
					rtsp_url = player->vir_rtsp_info->rtsp_url;
					log_debug("生成设备虚拟rtsp地址: %s", rtsp_url.c_str());
					log_info(log_queue, "生成设备虚拟rtsp地址: %s", rtsp_url.c_str());
				}
				else
				{
					ret = "连接到设备端超时";
					break;
				}
			}
			else
			{
				rtsp_url = player->vir_rtsp_info->rtsp_url;
				pthread_mutex_unlock(&player->lock);
				log_debug("已存在的订阅, 返回虚拟rtsp地址: %s", rtsp_url.c_str());
				log_info(log_queue, "已存在的订阅, 返回虚拟rtsp地址: %s", rtsp_url.c_str());
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

// 不需要，没有客户连接时自动取消
bool handle_undescribe(std::string url, std::string body, mg_connection *c, OnRspCallback rsp_callback)
{
	Json::Reader reader;
	Json::Value root;
	if(reader.parse(body.c_str(), root))
	{
		int deviceid = root["deviceid"].asInt();
		rsp_callback(c, "{\"result\":\"true\"}");
	}
	else
	{
		rsp_callback(c, "{\"result\":\"false\"}");
	}
	return true;
}

