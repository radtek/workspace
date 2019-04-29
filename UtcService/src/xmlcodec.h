// =====================================================================================
//	Copyright (C) 2018 by Jiaxing Shao.All rights reserved.
//	文 件 名:  xmlCodec.h
//	作    者:  Jiaxing Shao, 13315567369@163.com
//	版 本 号:  1.0
//	创建时间:  2018年08月16日 10时32分52秒
//	Compiler:  g++
//	描    述:  
// =====================================================================================

#ifndef __XMLCODEC_H_H_H
#define __XMLCODEC_H_H_H

#include "defines.h"
#include "functions.h"
#include "singleton_log.h"
#include <vector>
#include <map>
#include <list>
#include <string>
#include <iostream>
using namespace std;
#include <pthread.h>

#include "tinyxml2.h"
using namespace tinyxml2;

extern list<T_Message*> g_listWaitSendMsg;						// 等待发送到CCU的数据
extern pthread_mutex_t g_mutexWaitSendMsg;						// list互斥锁
extern pthread_cond_t g_condWaitSendMsg;						// list条件变量
	
extern map<unsigned int, list<T_Message*> > g_mapRecvMessage;	// CCU接收到的数据,主键为ccuIp的inet_aton转换值
extern map<unsigned int, pthread_mutex_t> g_mapRecvMessageLock;	// 互斥锁

extern list<string> g_listWaitDecodeXml;						// 存储接收到的xml数据 
extern pthread_mutex_t g_mutexWaitDecodeXml;

extern list<string> g_listWaitSendXml;							// 存储需要发送的xml数据
extern pthread_mutex_t g_mutexWaitSendXml;
extern pthread_cond_t g_condWaitSendXml;

extern vector<T_CcuInfo*> g_vecCcuInfo;						// ccu信息
extern map<unsigned int, T_LcInfo*> g_mapCrossInfo;			// 路口信息

/* *
 * 信号机相序转平台用相序,京通
 * */
const int Shape_Table_JT[] = {3,2,1,9,8,7,12,11,10,6,5,4,13,15,16,14,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32};
   
/* *
 * 信号机相序转平台用相序,双柏
 * */
const int Shape_Table_SB[] = {6,5,4,26,30,14,22,18,12,11,10,28,32,16,24,20,9,8,7,27,31,15,23,19,3,2,1,25,29,13,21,17};

// 转换为xml的顺序
const int Shape_Table_XML[] = {10, 11, 12, 1, 2, 3, 4, 5, 6, 7, 8, 9, 19, 21, 23, 17};

class XmlCodec
{
public:
	bool DecodeXmlMessage(string xmlStr);		// 解析xml字符串为buffer
	bool EncodeXmlMessage(T_Message *msg);		// 解析buffer为xml字符串
	int SubscribeMessage(char *buffer, int type);		// 订阅消息，存储在buffer
private:
};

#endif

