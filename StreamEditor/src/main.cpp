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
extern map<unsigned int, t_device_info*> g_mapDeviceInfo;
char g_localhost[16];

bool get_device_info();

int main(int argc, char *argv[])
{
	memset(g_localhost, 0, 16);
	char port[8] = { 0 };
	GetConfigureString("local.ipaddr", g_localhost, 16, "127.0.0.1", CONFFILE);
	GetConfigureString("http.Service", port, 8, "8000", CONFFILE);

	start_log_thread();
	sleep(1);
	log_queue = create_log_queue("rtsp_logs");
	if(!get_device_info())
	{
		return EXIT_FAILURE;
	}

	std::string httpport = port;
	auto http_server = std::shared_ptr<HttpServer>(new HttpServer);
	http_server->Init(httpport);
	http_server->AddHandler("/rtsp/describe", handle_describe);
	http_server->AddHandler("/rtsp/undescribe", handle_undescribe);
	http_server->Start();
	
	return EXIT_SUCCESS;
}

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
		
		string str = get_otl_conn(ora_user, ora_pwd, ora_ip, ora_port, ora_name);
		if(!database_open(str.c_str()))
		{
			log_info(log_queue, "connect oracle failed.");
			cout << str.c_str() << endl;
			log_debug("数据库连接失败, 程序退出, [%s]", str.c_str());
			break;
		}

		if(!select_device_info(g_mapDeviceInfo))
		{
			log_info(log_queue, "get device info failed.");
			log_debug("获取设备信息失败, 程序退出");
			break;
		}
		else
		{
			map<unsigned int, t_device_info*>::iterator iter;
			log_debug("获取设备信息成功");
			for(iter = g_mapDeviceInfo.begin(); iter != g_mapDeviceInfo.end(); iter++)
			{
				log_debug("DeviceID: %10d, DeviceType: %.1d, IP: %15s, Username: %8s, Password: %12s",
						iter->second->deviceid, iter->second->devicetype, iter->second->ipaddr,
						iter->second->username, iter->second->password);
			}
		}
		database_close();
		return true;
	}while(0);
	return false;
}

