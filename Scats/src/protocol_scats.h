// =====================================================================================
//	Copyright (C) 2018 by Jiaxing Shao.All rights reserved.
//	文 件 名:  protocol_scats.h
//	作    者:  Jiaxing Shao, 13315567369@163.com
//	版 本 号:  1.0
//	创建时间:  2018年06月08日 12时22分16秒
//	Compiler:  g++
//	描    述:  
// =====================================================================================

#ifndef __PROTOCOL_SCATS_H_H_H
#define __PROTOCOL_SCATS_H_H_H

#include "log.h"
#include "defines.h"
#include <pthread.h>
#include <string.h>
#include <arpa/inet.h>

#define LICENCE_SIZE 	128
#define USERNAME_SIZE	128
#define PASSWORD_SIZE 	16
#define RCVBUF_SIZE		2048

class ProtocolScats
{
public:
	ProtocolScats();
	~ProtocolScats();
	int Translate(char *buffer, int length, VoidPtr &ptrData);
public:
	int test_func(char *buffer, int type);
	void print_msg(BYTE type, void *ptrData);
public:
	int EncodeLicenceMsg(char *buffer);			// licence验证
	int EncodeUserVerify(char *buffer);			// 用户验证
	int EncodeEventLog(char *buffer, T_EventLog stuEventLog); 				// 发送文本消息到事件日志, 1
	int EncodeRegionDetail(char *buffer, int update = 1);					// 区域信息请求, 2
	int EncodeQueryDatetime(char *buffer); 									// 查询时间, 3
	int EncodeQueryControllerMemory(char *buffer, T_MemoryCtrlRequest stuMemCtrl, BYTE &seqNo);	// 控制器内存, 4
	int EncodeQueryControllerDetail(char *buffer, unsigned short siteId, BYTE &seqNo);			// 控制器详细内容, 4
	int EncodeQueryControllerExtend(char *buffer, unsigned short siteId, BYTE &seqNo);			// 控制器拓展, 5
	int EncodeActionList(char *buffer, T_ActionListRequest stuActionList);						// Action list, 9
	int EncodeGreenWindow(char *buffer, T_GreenWindowRequest stuGreenWindow, unsigned short &id);	// 特勤请求, id为特勤任务的ID,取消时需要的, 10
	int EncodeGreenWindowCancel(char *buffer, unsigned short requestId); 							// 取消特勤, 10
	int EncodeAlarmsRequest(char *buffer, T_AlarmRequest stuAlarm);				// 报警, 12
	int EncodeRedlightTime(char *buffer, T_RedlightTime_Request stuRedtime); 	// 红灯时间请求, 13
	int EncodeRampMetering(char *buffer); 										// 检测器数据请求, 13
	int EncodeBlockData(char *buffer);											// 拥堵数据请求, 14
	int EncodeLaneBlockData(char *buffer);										// 路口拥堵数据请求, 14
	int EncodeDetectorBlock(char *buffer, T_DetectBlockRequest stuDetectBlock);	// 检测器拥堵索引请求, 14
	int EncodeRouteRequest(char *buffer, T_RouteRequest stuRoute);				// Route请求, 15
	int EncodeANTTSRequest(char *buffer, T_ANTTSRequest stuANTTS);				// ANTTS请求, 16
	int EncodeRegionDetailExtend(char *buffer, int update = 1);					// region拓展, 22
	int EncodeLockPhaseOrder(char *buffer, T_LockPhaseRequest stuLockPhase);	// 锁定相位, 30
	int EncodeIntervene(char *buffer, T_Intervene stuIntervene);				// 操作干涉请求, 31
	int EncodeLaneStatus(char *buffer, T_LaneStatusRequest stuLaneStat);		// 路口状态请求, 32
	int EncodeStrategyMonitor(char *buffer, T_StrategyMonitor stuStraMonitor);	// 监测策略请求, 33
	int EncodeDetectorCount(char *buffer, int start = 1);						// 监测器数量请求, 35
	int EncodeRegisterService(char *buffer, T_RegisterService stuRegister); 	// 注册服务请求, 150
public:
	bool DecodeResLicenceMsg(char *buffer, T_LicenceVersion &stuVersion);			// 100
	bool DecodeResUserVerify(char *buffer);											// 101
	bool DecodeRegionDetail(char *buffer, T_RegionDetail &stuRegion);				// 区域信息反馈2, 22
	bool DecodeDatetime(char *buffer, T_Datetime &stuTime);							// 时间反馈, 3
	bool DecodeControllerMemory(char *buffer, T_Controller &stuController);			// 控制器内存, 4
	bool DecodeControllerDetail(char *buffer, T_Controller &stuController);			// 控制器详细, 4
	bool DecodeControllerExtend(char *buffer, T_ControllerExtend &stuExtend);		// 控制器拓展, 5
	bool DecodeActionList(char *buffer, T_ActionListResponse &stuActionList);		// action list反馈, 9
	bool DecodeGreenWindow(char *buffer, T_GreenWindowResponse &stuGreen);			// 特勤反馈, 10
	bool DecodeAlarmResponse(char *buffer, T_AlarmResponse &stuAlarm);				// 警报反馈, 12
	bool DecodeRampMetering(char *buffer, T_DetectorData &stuDetectData); 			// 检测器数据反馈, 13
	bool DecodeBlockData(char *buffer, T_BlockData &stuBlockData);					// 拥堵数据反馈, 14
	bool DecodeRouteResponse(char *buffer, T_RouteResponse &stuRoute);				// Route反馈, 15
	bool DecodeANTTSResponse(char *buffer, T_ANTTSResponse &stuANTTS);				// ANTTS反馈,16
	bool DecodeLockPhaseOrder(char *buffer);										// 相位锁定反馈, 30
	bool DecodeIntervene(char *buffer);												// 操作干涉反馈, 31 
	bool DecodeLaneStatus(char *buffer, T_LaneStatusResponse &stuLaneStatus);		// 路口状态反馈, 32
	bool DecodeStrategyMonitor(char *buffer);										// 监测策略反馈, 33
	bool DecodeDetectorCount(char *buffer, T_DetectorCountResponse stuDetectCount);	// 监测器数量反馈, 35
	bool DecodeRegisterService(char *buffer, T_RegisterService &stuRegService); 	// 注册服务反馈, 150
	bool DecodeErrorResponse(char *buffer);											// 错误应答, 255
private:
	void SetLaneStatusRequest(const T_LaneStatusRequest &data);			// 设置灯态请求信息
private:
	BYTE GetSequenceNo()
	{
		pthread_mutex_lock(&m_mutexSN);
		BYTE ch = m_SequenceNo;
		m_SequenceNo += 1;
		pthread_mutex_unlock(&m_mutexSN);
	}

	unsigned short GetGreenWindowId()
	{
		pthread_mutex_lock(&m_mutexGW);
		unsigned short id = m_greenWindowId;
		m_greenWindowId += 1;
		pthread_mutex_unlock(&m_mutexGW);
		return id;
	}
private:
	unsigned short m_greenWindowId;	// 特勤任务ID
	BYTE m_SequenceNo;				// 
	char m_szLicence[LICENCE_SIZE];	// licence
	char m_szUser[USERNAME_SIZE];	// scats系统用户名
	T_LicenceVersion m_version;		// 连接scats时，返回的版本信息等
	T_LaneStatusRequest *m_stuLaneStatusRequest;		// 灯态请求消息，数据解析时需根据请求消息解析数据
	pthread_mutex_t m_mutexLSR;		// 灯态请求信息的互斥锁
	pthread_mutex_t m_mutexSN;		// sequence number的互斥锁
	pthread_mutex_t m_mutexGW;		// greenWindow的互斥锁

	char m_szPwd[PASSWORD_SIZE];	// 
	BYTE m_userId;
	BYTE m_workstationId;
};

class VoidPtr
{
public:
	VoidPtr()
	{
		type = 0;
		data = NULL;
	}
	void SetPtr(int n);
	void DelPtr();
public:
	BYTE type;
	void *data;
};

#endif
