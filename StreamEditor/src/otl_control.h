// =====================================================================================
//	Copyright (C) 2019 by Jiaxing Shao.All rights reserved.
//	文 件 名:  otl_control.h
//	作    者:  Jiaxing Shao, 13315567369@163.com
//	版 本 号:  1.0
//	创建时间:  2019年04月10日 20时02分03秒
//	Compiler:  g++
//	描    述:  
// =====================================================================================

#ifndef _OTL_CONTROL_H_H_H
#define _OTL_CONTROL_H_H_H

#include "rtsp_struct.h"
#include <string>
#include <map>
using namespace std;
#include <pthread.h>
#define OTL_ORA11G_R2
#include "otlv4.h"

string get_otl_conn(char *username, char *password, char *ipaddr, char *port, char *dbname);
string get_otl_conn(char *username, char *password, char *dbname);
bool database_open(const char *conn_str);
bool database_close();
bool select_device_info(map<unsigned int, t_device_info*> &mapDeviceInfo);

#endif

