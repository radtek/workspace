// =====================================================================================
//	Copyright (C) 2018 by Jiaxing Shao.All rights reserved.
//	文 件 名:  defines.h
//	作    者:  Jiaxing Shao, 13315567369@163.com
//	版 本 号:  1.0
//	创建时间:  2018年08月15日 15时31分02秒
//	Compiler:  g++
//	描    述:  
// =====================================================================================

#ifndef __DEFINES_H_H_H
#define __DEFINES_H_H_H

#include <iostream>
using namespace std;
#include <string.h>

#define IP_SIZE			16
#define PORT_SIZE		8
#define BUF_HEAD_LEN 	7
#define BUF_LEN 		1024
#define RECONN_TIME		60

struct T_CcuInfo
{
	char ipaddr[IP_SIZE];		// ccu地址
	int port;		// ccu端口
	unsigned int inetAddr;		// 数字型IP
};

struct T_Message
{
	int crossId;				// 路口ID
	int type;					// 包类型, 3:主动上报, 4:查询回报，5:设置回报
	int buflen;					// 包长度
	char buffer[1024];			// 数据内容
};

struct T_LcInfo
{
	unsigned int crossId;
	char ipaddr[16];
	int port;
	int type;							// 信号机类型
	unsigned int ccuip_inetAddr;		// CCU ip地址,数字值
};

enum ENUM_PARSETYPE
{
	emDecode = 1,
	emEncode = 2
};

struct T_Task
{
	int type;
	string strXml;
	T_Message *msg;
};

#endif

