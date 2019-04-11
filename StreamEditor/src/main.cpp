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

#include "functions.h"
#include "logfile.h"
#include "otl_control.h"
#include "rtsp_struct.h"
#include "http_handle.h"

LOG_QUEUE *log_queue = NULL;
// 设备信息,deviceid为key
map<unsigned int, t_device_info*> g_mapDeviceInfo;
// 当前播放列表,deviceid为key
map<unsigned int, t_video_play_info*> g_mapVideoPlay;


bool get_device_info();


int main(int argc, char *argv[])
{
	start_log_thread();
	sleep(1);
	log_queue = create_log_queue("rtsp_logs");
	if(!get_device_info())
	{
		return EXIT_FAILURE;
	}

	std::string port = "8000";
	auto http_server = std::shared_ptr<HttpServer>(new HttpServer);
	http_server->Init(port);
	http_server->AddHandler("/rtsp/describe", handle_describe);
	http_server->AddHandler("/rtsp/undescribe", handle_undescribe);
	http_server->Start();
	
	return EXIT_SUCCESS;
}

bool get_device_info()
{
	do{
		string str = get_otl_conn("EHL_VIPS", "ehl1234", "37.79.2.5", "1521", "RACDB");
		if(!database_open(str.c_str()))
		{
			log_info(log_queue, "connect oracle failed.");
			break;
		}
		if(!select_device_info(g_mapDeviceInfo))
		{
			log_info(log_queue, "get device info failed.");
			break;
		}
		database_close();
		return true;
	}while(0);
	return false;
}
