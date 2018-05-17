// @Author:
// windows下需附加包含目录、附加库目录、附加依赖项(oci.lib)
// linux下需链接.so文件
// OTL应用于多线程时需要加锁

#ifndef _OTL_DATABASE_H_H
#define _OTL_DATABASE_H_H

#include "defines.h"
#include "log.h"
#include <pthread.h>
#define OTL_ORA11G_R2	//需定义在包含头文件 otlv4.h 之前
#include "otlv4.h"
#include <iostream>
#include <sstream>
using namespace std;

bool OTLConnect(const char * strConn);
bool OTLDisConnect();
void OTLReConnect(const char * strConn);

bool LogConnect(const char * strConn);
bool LogDisConnect();
void LogReConnect(const char * strConn);

int GetConfigureFromDatabase();						//获取站点信息
bool InitializeOtlInfo();							//获取数据库信息
bool insert_passcar_message(VmsData* msg);			//卡口数据入库
bool insert_recv_log(const stuLogInfo &logs);		//采集日志入库
bool insert_error_log(const stuLogInfo &logs);		//错误日志入库

#endif
