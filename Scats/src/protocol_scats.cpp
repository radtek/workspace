// =====================================================================================
//	Copyright (C) 2018 by Jiaxing Shao.All rights reserved.
//	文 件 名:  protocol_scats.cpp
//	作    者:  Jiaxing Shao, 13315567369@163.com
//	版 本 号:  1.0
//	创建时间:  2018年06月08日 12时22分21秒
//	Compiler:  g++
//	描    述:  
// =====================================================================================

#include "protocol_scats.h"

ProtocolScats::ProtocolScats()
{
	memset(m_szLicence, 0, LICENCE_SIZE);
	memset(m_szUser, 0, USERNAME_SIZE);
	memset(m_szPwd, 0, PASSWORD_SIZE);
	pthread_mutex_init(&m_mutexSN, NULL);
	pthread_mutex_init(&m_mutexGW, NULL);
	pthread_mutex_init(&m_mutexLSR, NULL);
	m_SequenceNo = 0;
	m_greenWindowId = 0;

	GetConfigureString("Username", m_szUser, USERNAME_SIZE, "Tyco ITS Data Extractor", CONFFILE);
	GetConfigureString("Licence", m_szLicence, LICENCE_SIZE, "wCN2KUlMnTbqxVDf3aLz5VzRPCMQZ_SV", CONFFILE);
	GetConfigureString("Password", m_szPwd, PASSWORD_SIZE, "passwd", CONFFILE);
	m_userId = 1;
	m_workstationId = 2;

	m_stuLaneStatusRequest = new T_LaneStatusRequest;
	memset(m_stuLaneStatusRequest, 0, sizeof(T_LaneStatusRequest));
}

ProtocolScats::~ProtocolScats()
{
	pthread_mutex_destroy(&m_mutexSN);
	pthread_mutex_destroy(&m_mutexGW);
	pthread_mutex_destroy(&m_mutexLSR);
}

/* *
 * 协议解析统一接口
 * */
int ProtocolScats::Translate(char *buffer, int length, VoidPtr &ptrData)
{
	if(buffer == NULL)
		return -1;

	bool result = false;
	VoidPtr ptrData;
	int strLen = 0;
	char tempStr[RCVBUF_SIZE * 3] = { 0 };

	switch((BYTE)buffer[2])
	{
		case 2:
		case 22:
			ptrData.SetPtr(2);
			result = DecodeRegionDetail(buffer, *(T_RegionDetail*)ptrData.data);
			break;
		case 3:
			ptrData.SetPtr(3);
			result = DecodeDatetime(buffer, *(T_Datetime*)ptrData.data);
			break;
		case 4:
			ptrData.SetPtr(4);
			// 一个一个测，需要seqNo,目前未实现
		//	result = DecodeControllerMemory(buffer, *(T_Controller*)ptrData.data);
			result = DecodeControllerDetail(buffer, *(T_Controller*)ptrData.data);
			break;
		case 5:
			ptrData.SetPtr(5);
			result = DecodeControllerExtend(buffer, *(T_ControllerExtend*)ptrData.data);
			break;
		case 9:
			ptrData.SetPtr(9);
			result = DecodeActionList(buffer, *(T_ActionListResponse*)ptrData.data);
			break;
		case 10:
			ptrData.SetPtr(10);
			result = DecodeGreenWindow(buffer, *(T_GreenWindowResponse*)ptrData.data);
			break;
		case 12:
			ptrData.SetPtr(12);
			result = DecodeAlarmResponse(buffer, *(T_AlarmResponse*)ptrData.data);
			break;
		case 13:
			ptrData.SetPtr(13);
			result = DecodeRampMetering(buffer, *(T_DetectorData*)ptrData.data);
			break;
		case 14:
			ptrData.SetPtr(14);
			result = DecodeBlockData(buffer, *(T_BlockData*)ptrData.data);
			break;
		case 16:
			ptrData.SetPtr(16);
			result = DecodeANTTSResponse(buffer, *(T_ANTTSResponse*)ptrData.data);
			break;
		case 30:
			result = DecodeLockPhaseOrder(buffer);
			break;
		case 31:
			result = DecodeIntervene(buffer);
			break;
		case 32:
			ptrData.SetPtr(32);
			result = DecodeLaneStatus(buffer, *(T_LaneStatusResponse*)ptrData.data);
			break;
		case 33:
			result = DecodeStrategyMonitor(buffer);
			break;
		case 35:
			ptrData.SetPtr(35);
			result = DecodeDetectorCount(buffer, *(T_DetectorCountResponse*)ptrData.data);
			break;
		case 150:
			ptrData.SetPtr(150);
			result = DecodeRegisterService(buffer, *(T_RegisterService*)ptrData.data);
			break;
		case 255:
			result = DecodeErrorResponse(buffer);
			break;
		default:
			strLen = ParseMessageHex(tempStr, buffer, length);
			g_logs->WriteWarn("Cannot decode message: %s", tempStr);
	}

	print_msg((BYTE)buffer[2], ptrData.data);
	ptrData.DelPtr();	// 删除内存
	return result;
}

/**********************************************************************************************************************************************/

/* *
 * 函数仅供测试使用
 * */
int ProtocolScats::test_func(char *buffer, int type)
{
	int length = 0;
	unsigned short requestId = 100;
	int siteId = 123;
	int update = 1;
	int start = 1;
	BYTE seqNo = 0;

	switch(type)
	{
		case emEventLog:
			T_EventLog stuEventLog;
			memset(&stuEventLog, 0, sizeof(T_EventLog));
			/************** Begin ***************/
//			stuEventLog.isFaultLogFlag = 1;
//			stuEventLog.isAlarmDisplayFlag = 1;
			stuEventLog.isEventLogFlag = 1;
			stuEventLog.isAlarmTypeFlag = 1;
//			stuEventLog.isFaultFlag = 1;
			stuEventLog.alarmId = 255;
			stuEventLog.extendAlarmId = 1024;
			memcpy(stuEventLog.text, "Hello World!", 12);
			/************** End ***************/
			length = EncodeEventLog(buffer, stuEventLog);break;

		case emRegionDetail:
			length = EncodeRegionDetail(buffer, update);break;

		case emQueryDatetime:
			length = EncodeQueryDatetime(buffer);break;

		case emControllerMemory:
			T_MemoryCtrlRequest stuMemCtrl;
			memset(&stuMemCtrl, 0, sizeof(T_MemoryCtrlRequest));
			/************** Begin ***************/
			stuMemCtrl.siteId = 100;
			stuMemCtrl.page = 100;
			stuMemCtrl.offset = 100;
			/************** End ***************/
			length = EncodeQueryControllerMemory(buffer, stuMemCtrl, seqNo);break;

		case emQueryControllerDetail:
			length = EncodeQueryControllerDetail(buffer, siteId, seqNo);break;

		case emQueryControllerExtend:
			length = EncodeQueryControllerExtend(buffer, siteId, seqNo);break;

		case emActionList:
			T_ActionListRequest stuActionList;
			memset(&stuActionList, 0, sizeof(T_ActionListRequest));
			/************** Begin ***************/
			stuActionList.mode = 0;
			memcpy(stuActionList.regionname, "Noname", 6);
			stuActionList.siteId = 1024;
			stuActionList.listCount = 10;
			for(int i = 0; i < stuActionList.listCount; i++)
			{
				stuActionList.list[i] = 128;
			}
			/************** End ***************/
			length = EncodeActionList(buffer, stuActionList);break;

		case emGreenWindow:
			T_GreenWindowRequest stuGreenWindow;
			memset(&stuGreenWindow, 0, sizeof(T_GreenWindowRequest));
			/************** Begin ***************/
			stuGreenWindow.priority = 16;
			stuGreenWindow.count = 1;
			for(int i = 0; i < stuGreenWindow.count; i++)
			{
				stuGreenWindow.site[i].id = 13300 + i;
				stuGreenWindow.site[i].starttime = 1240;
				stuGreenWindow.site[i].duration = 10;
				stuGreenWindow.site[i].phase = 120;
			}
			/************** End ***************/
			length = EncodeGreenWindow(buffer, stuGreenWindow, requestId);break;

		case emAlarmDisConn:
			T_AlarmRequest stuAlarm;
			memset(&stuAlarm, 0, sizeof(T_AlarmRequest));
			/************** Begin ***************/
			stuAlarm.subType = 0;
			stuAlarm.userId = 0;
			/************** End ***************/
			length = EncodeAlarmsRequest(buffer, stuAlarm);break;

		case emAlarmConn:
			memset(&stuAlarm, 0, sizeof(T_AlarmRequest));
			/************** Begin ***************/
			stuAlarm.subType = 1;
			stuAlarm.userId = 0;
			/************** End ***************/
			length = EncodeAlarmsRequest(buffer, stuAlarm);break;

		case emAlarmAck:
			memset(&stuAlarm, 0, sizeof(T_AlarmRequest));
			/************** Begin ***************/
			stuAlarm.subType = 2;
			stuAlarm.userId = 0;
			/************** End ***************/
			length = EncodeAlarmsRequest(buffer, stuAlarm);break;

		case emAlarmHide:
			memset(&stuAlarm, 0, sizeof(T_AlarmRequest));
			/************** Begin ***************/
			stuAlarm.subType = 3;
			stuAlarm.userId = 0;
			/************** End ***************/
			length = EncodeAlarmsRequest(buffer, stuAlarm);break;

		case emAlarmShow:
			memset(&stuAlarm, 0, sizeof(T_AlarmRequest));
			/************** Begin ***************/
			stuAlarm.subType = 5;
			stuAlarm.userId = 0;
			/************** End ***************/
			length = EncodeAlarmsRequest(buffer, stuAlarm);break;

		case emAlarmClear:
			memset(&stuAlarm, 0, sizeof(T_AlarmRequest));
			/************** Begin ***************/
			stuAlarm.subType = 8;
			stuAlarm.userId = 0;
			/************** End ***************/
			length = EncodeAlarmsRequest(buffer, stuAlarm);break;

		case emAlarmDelete:
			memset(&stuAlarm, 0, sizeof(T_AlarmRequest));
			/************** Begin ***************/
			stuAlarm.subType = 9;
			stuAlarm.userId = 0;
			/************** End ***************/
			length = EncodeAlarmsRequest(buffer, stuAlarm);break;

		case emGreenWindowCancel:
			length = EncodeGreenWindowCancel(buffer, requestId);break;

		case emRampMetering:
			length = EncodeRampMetering(buffer);break;

		case emRedlightTime:
			T_RedlightTime_Request stuRedtime;
			memset(&stuRedtime, 0, sizeof(T_RedlightTime_Request));
			/************** Begin ***************/
			stuRedtime.count = 2;
			for(int i = 0; i < stuRedtime.count; i++)
			{
				stuRedtime.ramp[i].rampNo = i;
				stuRedtime.ramp[i].siteId = 13308 + i;
				stuRedtime.ramp[i].redtime = 10;
			}
			/************** End ***************/
			length = EncodeRedlightTime(buffer, stuRedtime);break;

		case emIntervene:
			T_Intervene stuIntervene;
			memset(&stuIntervene, 0, sizeof(T_Intervene));
			/************** Begin ***************/
			stuIntervene.mode = 0;
			stuIntervene.userId = 10;
			memcpy(stuIntervene.regionname, "Nobody", 6);
			stuIntervene.subsysNo = 10;
			stuIntervene.siteId = 10086;
			/************** End ***************/
			length = EncodeIntervene(buffer, stuIntervene);break;

		case emLockPhaseOrder:
			T_LockPhaseRequest stuLockPhase;
			memset(&stuLockPhase, 0, sizeof(T_LockPhaseRequest));
			/************** Begin ***************/
			stuLockPhase.siteId = 10086;
			stuLockPhase.preferredPhase = 6;
			stuLockPhase.alternatePhase = 0;
			stuLockPhase.time = 30;
			/************** End ***************/
			length = EncodeLockPhaseOrder(buffer, stuLockPhase);break;

		case emCongestion:
			length = EncodeBlockData(buffer);break;

		case emCongestionIndex:
			length = EncodeLaneBlockData(buffer);break;

		case emDetectorCongestionIndex:
			T_DetectBlockRequest stuDetectBlock;
			memset(&stuDetectBlock, 0, sizeof(T_DetectBlockRequest));
			/************** Begin ***************/
			stuDetectBlock.count = 0;
			for(int i = 0; i < stuDetectBlock.count; i++)
			{
				stuDetectBlock.siteIds[i] = i + 13300;
			}
			/************** End ***************/
			length = EncodeDetectorBlock(buffer, stuDetectBlock);break;
		
		case emRegionDetailExtend:
			length = EncodeRegionDetailExtend(buffer, update);break;

		case emLaneStatus:
			T_LaneStatusRequest stuLaneStatus;
			memset(&stuLaneStatus, 0, sizeof(T_LaneStatusRequest));
			/************** Begin ***************/
			stuLaneStatus.stMsgcode.bStart = true;
			stuLaneStatus.stMsgcode.bDefaultDataType = true;
			stuLaneStatus.stMsgcode.bSiteSpecific = false;
			stuLaneStatus.stMsgcode.byMsgSubtype = 1;

			// bit 0-5
			stuLaneStatus.stDefaultDataType.bHasSiteStatus = true;
			stuLaneStatus.stDefaultDataType.bHasPlanNumber = true;
			stuLaneStatus.stDefaultDataType.bHasPhaseData = true;
			stuLaneStatus.stDefaultDataType.bHasFlagsData = true;
			stuLaneStatus.stDefaultDataType.bHasSignalGroupData = true;
			stuLaneStatus.stDefaultDataType.bHasSubsysData = true;

			// bit 8-10
			stuLaneStatus.stDefaultDataType.bHasAlarmDetail = true;
			stuLaneStatus.stDefaultDataType.bHasPhaseChanges = true;
			stuLaneStatus.stDefaultDataType.bHasPhaseInterval = true;

			// bit 14-15
			stuLaneStatus.stDefaultDataType.bHasResponse = false;
			stuLaneStatus.stDefaultDataType.bHasSendToAll = false;

			stuLaneStatus.iCount = 1;
			for(int i = 0; i < stuLaneStatus.iCount; i++)
			{
				stuLaneStatus.stSitedata[i].id = 9037;
				if(stuLaneStatus.stMsgcode.bSiteSpecific)
				{
					stuLaneStatus.stSitedata[i].stSsdt.bHasSiteStatus = true;
					stuLaneStatus.stSitedata[i].stSsdt.bHasPlanNumber = true;
					stuLaneStatus.stSitedata[i].stSsdt.bHasPhaseData = true;
					stuLaneStatus.stSitedata[i].stSsdt.bHasFlagsData = true;
					stuLaneStatus.stSitedata[i].stSsdt.bHasSignalGroupData = true;
					stuLaneStatus.stSitedata[i].stSsdt.bHasSubsysData = true;
					stuLaneStatus.stSitedata[i].stSsdt.bHasAlarmDetail = true;
					stuLaneStatus.stSitedata[i].stSsdt.bHasPhaseChanges = true;
					stuLaneStatus.stSitedata[i].stSsdt.bHasPhaseInterval = true;
					stuLaneStatus.stSitedata[i].stSsdt.bHasSendToAll = true;
				}
			}
			/************** End ***************/
			SetLaneStatusRequest(stuLaneStatus);
			length = EncodeLaneStatus(buffer, stuLaneStatus);break;

		case emStrategyMonitor:
			T_StrategyMonitor stuStraMonitor;
			memset(&stuStraMonitor, 0, sizeof(T_StrategyMonitor));
			/************** Begin ***************/
			stuStraMonitor.isSM = true;
			stuStraMonitor.noRepeat = true;
			memcpy(stuStraMonitor.regionname, "Nobody", 6);
			/************** End ***************/
			length = EncodeStrategyMonitor(buffer, stuStraMonitor);break;

		case emDetectorCount:
			length = EncodeDetectorCount(buffer, start);break;

		case emRegisterService:
			T_RegisterService stuRegister;
			memset(&stuRegister, 0, sizeof(T_RegisterService));
			/************** Begin ***************/
			stuRegister.dataId1 = 151;
			stuRegister.dataId2 = 152;
			stuRegister.dataId3 = 153;
			stuRegister.dataId4 = 154;
			/************** End ***************/
			length = EncodeRegisterService(buffer, stuRegister);break;

		default:
			cout << "No this command!" << endl;
	}
	return length;
}

/**********************************************************************************************************************************************/

/* *
 * 连接到scats之后需要在10秒内发送Licence
 * */
int ProtocolScats::EncodeLicenceMsg(char *buffer)
{
	if(buffer == NULL)
		return -1;

	buffer[2] = 100 & 0xFF;
	sprintf(buffer + 3, "%s\\%s", m_szUser, m_szLicence);
//	memcpy(buffer + 3, m_szLicence, strlen(m_szUser));
//	memcpy(buffer + 3, m_szLicence, strlen(m_szLicence));
	unsigned short length = strlen(m_szLicence) + strlen(m_szUser) + 3 + 1;
	*((unsigned short*)buffer) = htons(length - 2);
	return int(length);
}

/* *
 * 
 * */
int ProtocolScats::EncodeUserVerify(char *buffer)
{
	if(buffer == NULL)
		return -1;

	buffer[2] = 101 & 0xFF;
	buffer[3] = m_workstationId;
	buffer[4] = GetSequenceNo();
	buffer[5] = m_userId;
	memcpy(buffer + 6, m_szPwd, strlen(m_szPwd));
	unsigned short length = strlen(m_szPwd) + 4 + 2;
	*((unsigned short*)buffer) = htons(length - 2);
	return int(length);
}

/* *
 * 向scats写日志
 * */
int ProtocolScats::EncodeEventLog(char *buffer, T_EventLog stuEventLog)
{
	if(buffer == NULL)
		return -1;

	buffer[2] = 1;
	buffer[3] = 16;
	buffer[4] = (stuEventLog.isFaultLogFlag)?1:0;
	buffer[4] += (stuEventLog.isAlarmDisplayFlag)?((1 << 1) & 0x02):0;
	buffer[4] += (stuEventLog.isEventLogFlag)?((1 << 2) & 0x04):0;
	buffer[4] += (stuEventLog.isAlarmTypeFlag)?((1 << 4) & 0x10):0;
	buffer[4] += (stuEventLog.isFaultFlag)?((1 << 6) & 0x40):0;
	buffer[4] += (1 << 7) & 0x80;

	unsigned short length = 3;
	if(stuEventLog.isAlarmTypeFlag)
	{
		buffer[5] = stuEventLog.alarmId;
		length += 1;
		if(stuEventLog.alarmId == 0xFF)
		{
			*((unsigned short*)(buffer + 6)) = htons(stuEventLog.extendAlarmId);
			length += 2;
		}
	}
	memcpy(buffer + length, stuEventLog.text, strlen(stuEventLog.text));
	length += strlen(stuEventLog.text);
	*((unsigned short*)buffer) = htons(length - 2);
	return length;
}

/* *
 * 获取区域详细信息,update: 1,当信息发生变化时发送更新
 * */
int ProtocolScats::EncodeRegionDetail(char *buffer, int update)
{
	if(buffer == NULL)
		return -1;

	buffer[2] = 2;
	if(update)
		buffer[3] = 1;
	else
		buffer[3] = 0;

	*((unsigned short*)buffer) = htons(2);
	return 2 + 2;
}

/* *
 * 查询时间
 * */
int ProtocolScats::EncodeQueryDatetime(char *buffer)
{
	if(buffer == NULL)
		return -1;

	buffer[2] = 3;
	*((unsigned short*)buffer) = htons(1);
	return 1 + 2;
}

/* *
 * 查询内存
 * */
int ProtocolScats::EncodeQueryControllerMemory(char *buffer, T_MemoryCtrlRequest stuMemCtrl, BYTE &seqNo)
{
	if(buffer == NULL)
		return -1;
	buffer[2] = 4;
	buffer[3] = GetSequenceNo();
	seqNo = buffer[3];
	*(unsigned short*)(buffer + 4) = htons(stuMemCtrl.siteId);
	buffer[6] = stuMemCtrl.page;
	buffer[7] = stuMemCtrl.offset;

	*((unsigned short*)buffer) = htons(6);
	return 8;
}

/* *
 * 
 * */
int ProtocolScats::EncodeQueryControllerDetail(char *buffer, unsigned short siteId, BYTE &seqNo)
{
	if(buffer == NULL)
		return -1;
	buffer[2] = 4;
	buffer[3] = GetSequenceNo();
	seqNo = buffer[3];
	*(unsigned short*)(buffer + 4) = htons(siteId);
	buffer[6] = 0xFF;
	*((unsigned short*)buffer) = htons(5);
	return 7;
}

/* *
 * 查询内存扩展信息 
 * */
int ProtocolScats::EncodeQueryControllerExtend(char *buffer, unsigned short siteId, BYTE &seqNo)
{
	if(buffer == NULL)
		return -1;
	buffer[2] = 5;
	buffer[3] = GetSequenceNo();
	seqNo = buffer[3];
	*(unsigned short*)(buffer + 4) = htons(siteId);
	*((unsigned short*)buffer) = htons(4);
	return 7;
}

/* *
 * 
 * */
int ProtocolScats::EncodeActionList(char *buffer, T_ActionListRequest stuActionList)
{
	if(buffer == NULL)
		return -1;

	buffer[2] = 9;
	buffer[3] = stuActionList.mode;
	int pos = 4;
	if(stuActionList.mode)
	{
		*(unsigned short*)(buffer + pos) = htons(stuActionList.siteId);
		pos += 2;
	}
	else
	{
		memcpy(buffer + pos, stuActionList.regionname, 6);
		pos += 6;
	}

	for(int i = 0; i < stuActionList.listCount; i++)
	{
		*((unsigned short*)(buffer+pos)) = htons(stuActionList.list[i]);
		pos += 2;
	}
	*((unsigned short*)buffer) = htons(pos - 2);
	return pos;
}

/* *
 * 执行特勤，会返回id的值，id值用于结束此id的特勤任务
 * */
int ProtocolScats::EncodeGreenWindow(char *buffer, T_GreenWindowRequest stuGreenWindow,unsigned short &id)
{
	if(buffer == NULL)
		return -1;

	buffer[2] = 10;
	buffer[3] = stuGreenWindow.priority;
	id = GetGreenWindowId();
	*(unsigned short*)(buffer + 4) = htons(id);	
	
	int length = 6;
	for(int i; i < stuGreenWindow.count; i++)
	{
		*(unsigned short*)(buffer + length) = htons(stuGreenWindow.site[i].id);
		length += 2;
		*(unsigned short*)(buffer + length) = htons(stuGreenWindow.site[i].starttime);
		length += 2;
		*(unsigned short*)(buffer + length) = htons(stuGreenWindow.site[i].duration);
		length += 2;
		buffer[length] = stuGreenWindow.site[i].phase;
		length += 1;
	}
	*((unsigned short*)buffer) = htons(length - 2);
	return length;
}

/* *
 * 取消特勤任务，需要传入requestId，requestId是执行特勤时的ID号
 * */
int ProtocolScats::EncodeGreenWindowCancel(char *buffer, unsigned short requestId)
{
	if(buffer == NULL)
		return -1;

	buffer[2] = 10;
	buffer[3] = 0x00;
	*((unsigned short*)(buffer+4)) = htons(requestId);
	*(unsigned short*)(buffer+6) = htons(65535);
	*((unsigned short*)buffer) = htons(6);
	return 8;
}


/* *
 * 警报请求,当不再使用警报器时应该关掉
 * */
int ProtocolScats::EncodeAlarmsRequest(char *buffer, T_AlarmRequest stuAlarm)
{
	if(buffer == NULL)
		return -1;

	buffer[2] = 12;
	buffer[3] = stuAlarm.subType;
	int length = 4;

	if(stuAlarm.subType > 1)
	{
		length = 8;
		buffer[4] = stuAlarm.userId;
		for(int i = 0; i < stuAlarm.idCount; i++)
		{
			*(unsigned long*)(buffer + length) = htonl(stuAlarm.alarmId[i]);
			length += 4;
		}
	}
	*((unsigned short*)buffer) = htons(length - 2);
	return length;
}

/* *
 * 红灯时间，似乎没有应答？
 * */
int ProtocolScats::EncodeRedlightTime(char *buffer, T_RedlightTime_Request stuRedtime)
{
	if(buffer == NULL)
		return -1;

	buffer[2] = 13;
	buffer[3] = 2;
	buffer[4] = GetSequenceNo();

	int length = 5;
	for(int i = 0; i < stuRedtime.count; i++)
	{
		*(unsigned int*)(buffer + length) = htons(stuRedtime.ramp[i].rampNo);
		*(unsigned int*)(buffer + length) += htons((stuRedtime.ramp[i].siteId << 8) & 0xFFFF);
		length += 4;
		*(unsigned short*)(buffer + length) = htons(stuRedtime.ramp[i].redtime);
		length += 2;
	}
	*((unsigned short*)buffer) = htons(length - 2);
	return length;
}

/* *
 * request 子类型是3的 response 子类型是1
 * */
int ProtocolScats::EncodeRampMetering(char *buffer)
{
	if(buffer == NULL)
		return -1;

	buffer[2] = 13;
	buffer[3] = 3;
	buffer[4] = GetSequenceNo();
	*((unsigned short*)buffer) = htons(3);
	return 5;
}

/* *
 * 拥堵数据,
 * */
int ProtocolScats::EncodeBlockData(char *buffer)
{
	if(buffer == NULL)
		return -1;
	buffer[2] = 14;
	buffer[3] = 1;
	*((unsigned short*)buffer) = htons(2);
	return 4;
}

/* *
 * 
 * */
int ProtocolScats::EncodeLaneBlockData(char *buffer)
{
	if(buffer == NULL)
		return -1;

	buffer[2] = 14;
	buffer[3] = 2;
	*((unsigned short*)buffer) = htons(2);
	return 4;
}

/* *
 * 
 * */
int ProtocolScats::EncodeDetectorBlock(char *buffer, T_DetectBlockRequest stuDetectBlock)
{
	if(buffer == NULL)
		return -1;

	buffer[2] = 14;
	buffer[3] = 3;
	int length = 4;
	for(int i = 0; i < stuDetectBlock.count; i++)
	{
		*((unsigned short*)(buffer + length)) = htons(stuDetectBlock.siteIds[i]);
		length += 2;
	}
	*((unsigned short*)buffer) = htons(length - 2);
	return length;
}

/* *
 * 绿波带
 * */
int ProtocolScats::EncodeRouteRequest(char *buffer, T_RouteRequest stuRoute)
{
	if(buffer == NULL)
		return -1;

	buffer[2] = 15;
	buffer[3] = GetSequenceNo();
	buffer[4] = stuRoute.workstation;
	buffer[5] = stuRoute.connFlag;
	buffer[6] = stuRoute.userId;
	buffer[7] = stuRoute.subtype;
	*(unsigned short*)(buffer + 8) = htons(stuRoute.routeNumber);
	*(unsigned short*)(buffer + 10) = htons(stuRoute.siteId);
	*((unsigned short*)buffer) = htons(10);
	return 12;
}

/* *
 * ANTTS消息
 * */
int ProtocolScats::EncodeANTTSRequest(char *buffer, T_ANTTSRequest stuANTTS)
{
	if(buffer == NULL)
		return -1;

	buffer[2] = 16;
	*((unsigned short*)buffer) = htons(1);
	return 3;
}

/* *
 * 请求带扩展的区域详细信息,update: 1,当信息发生变化时发送更新
 * */
int ProtocolScats::EncodeRegionDetailExtend(char *buffer, int update)
{
	if(buffer == NULL)
		return -1;

	buffer[2] = 22;
	if(update)
		buffer[3] = 1;
	else
		buffer[3] = 0;

	*((unsigned short*)buffer) = htons(2);
	return 2 + 2;
}

/* *
 * 
 * */
int ProtocolScats::EncodeLockPhaseOrder(char *buffer, T_LockPhaseRequest stuLockPhase)
{
	if(buffer == NULL)
		return -1;

	buffer[2] = 30;
	buffer[3] = GetSequenceNo();
	buffer[4] = m_userId;
	*(unsigned short*)(buffer + 5) = htons(stuLockPhase.siteId);
	buffer[7] = stuLockPhase.preferredPhase;
	buffer[8] = stuLockPhase.alternatePhase;
	*(unsigned short*)(buffer + 9) = htons(stuLockPhase.time);
	*((unsigned short*)buffer) = htons(7);
	return 9;
}

/* *
 * 
 * */
int ProtocolScats::EncodeIntervene(char *buffer, T_Intervene stuIntervene)
{
	if(buffer == NULL)
		return -1;

	buffer[2] = 31 & 0xFF;
	buffer[3] = GetSequenceNo();
	buffer[4] = stuIntervene.mode;
	buffer[5] = m_userId;
	unsigned short length = 4;

	if(buffer[4] & 0x01 == 0)
	{
		memcpy(buffer + 4, stuIntervene.regionname, 6);		// region name,6
		buffer[10] = stuIntervene.subsysNo;					// subsystem no,1
		length += 7;
	}
	else
	{
		*((unsigned short*)(buffer+4)) = htons(stuIntervene.siteId);
		length += 2;
	}

	int pos = length;
	if(stuIntervene.cFlag == 1)
	{
		buffer[pos] += 0x01;
		if(buffer[pos] == 0x00)
		{
			length += 1;
		}
		*(unsigned short*)(buffer + length) = htons(stuIntervene.cycleLen);
		length += 2;
		*(unsigned short*)(buffer + length) = htons(stuIntervene.cLocktime);
		length += 2;
	}
	if(stuIntervene.sFlag == 1)
	{
		buffer[pos] += (1 << 1) & 0x02;
		if(buffer[pos] == 0x00)
		{
			length += 1;
		}
		buffer[length] = stuIntervene.subsystemPlan;
		length += 1;
		*(unsigned short*)(buffer + length) = htons(stuIntervene.cLocktime);
		length += 2;
	}
	if(stuIntervene.lFlag == 1)
	{
		buffer[pos] += (1 << 2) & 0x04;
		if(buffer[pos] == 0x00)
		{
			length += 1;
		}
		buffer[length] = stuIntervene.linkPlan;
		length += 1;
		*(unsigned short*)(buffer + length) = htons(stuIntervene.cLocktime);
		length += 2;
	}
	if(stuIntervene.mdFlag == 1)
	{
		buffer[pos] += (1 << 3) & 0x08;
		if(buffer[pos] == 0x00)
		{
			length += 1;
		}
		buffer[length] = stuIntervene.mdSelection;
		length += 1;
		*(unsigned short*)(buffer + length) = htons(stuIntervene.cLocktime);
		length += 2;
	}

	*((unsigned short*)buffer) = length - 2;
	return length;
}

/* *
 * 
 * */
int ProtocolScats::EncodeLaneStatus(char *buffer, T_LaneStatusRequest stuLaneStat)
{
	if(buffer == NULL)
		return -1;

	T_LaneStatusRequest *request = new T_LaneStatusRequest;
	memcpy(request, &stuLaneStat, sizeof(T_LaneStatusRequest));

	buffer[2] = 32;
	buffer[3] = request->stMsgcode.bStart & 0x01;
	buffer[3] += (request->stMsgcode.bDefaultDataType & 0x01) << 1;
	buffer[3] += (request->stMsgcode.bSiteSpecific & 0x01) << 2;
	buffer[3] += (request->stMsgcode.byMsgSubtype & 0x03) << 4;

	int length = 4;
	if(request->stMsgcode.bDefaultDataType)
	{
		unsigned short temp = 0;
		temp += request->stDefaultDataType.bHasSiteStatus & 0x01;
		temp += (request->stDefaultDataType.bHasPlanNumber & 0x01) << 1;
		temp += (request->stDefaultDataType.bHasPhaseData & 0x01) << 2;
		temp += (request->stDefaultDataType.bHasFlagsData & 0x01) << 3;
		temp += (request->stDefaultDataType.bHasSignalGroupData & 0x01) << 4;
		temp += (request->stDefaultDataType.bHasSubsysData & 0x01) << 5;
		temp += (request->stDefaultDataType.bHasAlarmDetail & 0x01) << 8;
		temp += (request->stDefaultDataType.bHasPhaseChanges & 0x01) << 9;
		temp += (request->stDefaultDataType.bHasPhaseInterval & 0x01) << 10;
		temp += (request->stDefaultDataType.bHasResponse & 0x01) << 14;
		temp += (request->stDefaultDataType.bHasSendToAll & 0x01) << 15;
		*(unsigned short*)(buffer + length) = htons(temp);
		length += 2;
	}
	for(int i = 0; i < request->iCount; i++)
	{
		*(unsigned short*)(buffer + length) = htons(request->stSitedata[i].id);
		length += 2;
		if(request->stMsgcode.bSiteSpecific)
		{
			unsigned short temp = 0;
			temp += request->stSitedata[i].stSsdt.bHasSiteStatus & 0x01;
			temp += (request->stSitedata[i].stSsdt.bHasPlanNumber & 0x01) << 1;
			temp += (request->stSitedata[i].stSsdt.bHasPhaseData & 0x01) << 2;
			temp += (request->stSitedata[i].stSsdt.bHasFlagsData & 0x01) << 3;
			temp += (request->stSitedata[i].stSsdt.bHasSignalGroupData & 0x01) << 4;
			temp += (request->stSitedata[i].stSsdt.bHasSubsysData & 0x01) << 5;
			temp += (request->stSitedata[i].stSsdt.bHasAlarmDetail & 0x01) << 8;
			temp += (request->stSitedata[i].stSsdt.bHasPhaseChanges & 0x01) << 9;
			temp += (request->stSitedata[i].stSsdt.bHasPhaseInterval & 0x01) << 10;
			temp += (request->stSitedata[i].stSsdt.bHasSendToAll & 0x01) << 15;
			*(unsigned short*)(buffer + length) = htons(temp);
			length += 2;
		}
	}

	if(request != NULL)
	{
		delete request;
		request = NULL;
	}

	*((unsigned short*)buffer) = htons(length - 2);
	return length;
}

/* *
 * 
 * */
int ProtocolScats::EncodeStrategyMonitor(char *buffer, T_StrategyMonitor stuStraMonitor)
{
	if(buffer == NULL)
		return -1;

	buffer[2] = 33;
	if(stuStraMonitor.isSM)
	{
		buffer[3] = 1;
	}
	else
	{
		buffer[3] = 0;
	}

	if(stuStraMonitor.noRepeat)
	{
		buffer[3] += (1 << 1);
	}
	memcpy(buffer + 4, stuStraMonitor.regionname, 6);
	int length = 10;
	for(int i = 0; i < 32; i++)
	{
		buffer[length] = stuStraMonitor.subSystem[i];
		length += 1;
	}
	*((unsigned short*)buffer) = htons(length - 2);
	return length;
}

/* *
 * 
 * */
int ProtocolScats::EncodeDetectorCount(char *buffer, int start)
{
	if(buffer == NULL)
		return -1;

	buffer[2] = 35;
	if(start)
	{
		buffer[3] = 1;
	}
	else
	{
		buffer[3] = 0;
	}
	*((unsigned short*)buffer) = htons(2);
	return 4;
}

/* *
 * 
 * */
int ProtocolScats::EncodeRegisterService(char *buffer, T_RegisterService stuRegister)
{
	if(buffer == NULL)
		return -1;

	buffer[2] = 150;
	buffer[3] = stuRegister.dataId1;
	buffer[4] = stuRegister.dataId2;
	buffer[5] = stuRegister.dataId3;
	buffer[6] = stuRegister.dataId4;
	*((unsigned short*)buffer) = htons(5);
	return 7;
}

/* ****************************************************************************************************************************************** */

/* *
 * 
 * */
bool ProtocolScats::DecodeResLicenceMsg(char *buffer, T_LicenceVersion &stuVersion)
{
	if(buffer == NULL)
		return false;
	if(buffer[2] != 100)
		return false;
	int buflen = ntohs(*((unsigned short*)buffer));

	bool result = false;
	switch((BYTE)buffer[3])
	{
		case 0:
			result = true;break;
		case 1:
			LogInfo.WriteErr("LicenceResponse Username/Licence 语法错误.");break;
		case 2:
			LogInfo.WriteErr("LicenceResponse 无效的Licence.");break;
		case 3:
			LogInfo.WriteErr("LicenceResponse 已经登陆.");break;
		case 4:
			LogInfo.WriteErr("LicenceResponse 没有被使用.");break;
		case 5:
			LogInfo.WriteErr("LicenceResponse 没有注册或已经被禁用的Username.");break;
		case 6:
			LogInfo.WriteErr("LicenceResponse Connection instance limit reached.");break;
		default:
			LogInfo.WriteErr("LicenceResponse undefined error!");break;
	}

	int offset = 0;
	if(result)
	{
		stuVersion.protocol = buffer[4];
		offset = 1;
	}

	short ver = ntohs(*((short*)(buffer + 5 + offset)));
	stuVersion.version.minor = ver & 0x3F;
	stuVersion.version.major = (ver >> 6) & 0x3F;
	stuVersion.version.main = (ver >> 12) & 0x0F;
	stuVersion.hostNumber = buffer[6 + offset];

	return result;
}

/* *
 * 
 * */
bool ProtocolScats::DecodeResUserVerify(char *buffer)
{
	if(buffer == NULL)
		return false;

	if(buffer[2] != 101)
		return false;
	int buflen = ntohs(*((unsigned short*)buffer));

	if((BYTE)buffer[6] <= 7)
	{
		return true;
	}
	else if((BYTE)buffer[6] == 254)
	{
		g_logs->WriteErr("UserVerifyResponse password error!");
		return false;
	}
	else
	{
		return false;
	}
}

/* *
 * 
 * */
bool ProtocolScats::DecodeRegionDetail(char *buffer, T_RegionDetail &stuRegion)
{
	if(buffer == NULL)
		return false;
	int buflen = ntohs(*((unsigned short*)buffer));
	if((BYTE)buffer[2] != 2 || (BYTE)buffer[2] != 22)
		return false;

	stuRegion.regionNo = buffer[3];
	int pos = 4;
	if(stuRegion.regionNo <= 64)
	{
		short ver = ntohs(*((short*)(buffer+4)));
		stuRegion.version.minor = ver & 0x3F;
		stuRegion.version.major = (ver >> 6) & 0x3F;
		stuRegion.version.main = (ver >> 12) & 0x0F;
		pos += 2;
	}

	memcpy(stuRegion.regionName, buffer + pos, 6);
	pos += 6;
	short opt = ntohs(*((unsigned short*)(buffer + pos)));
	pos += 2;
	stuRegion.support.isFixedTime = opt & 0x01;
	stuRegion.support.isAdaptive = (opt >> 1) & 0x01;
	stuRegion.support.isCMS = (opt >> 2) & 0x01;
	stuRegion.support.isDIDO = (opt >> 3) & 0x01;
	stuRegion.support.isANTTS = (opt >> 4) & 0x01;
	stuRegion.ipaddr = ntohl(*(int*)(buffer + pos));
	pos += 4;
	stuRegion.port = ntohs(*(short*)(buffer + pos));
	pos += 2;

	if((BYTE)buffer[2] == 22)
	{
		stuRegion.isHave = true;
		stuRegion.count = buffer[pos];
		pos += 1;
		stuRegion.buildNo = buffer[pos];
		pos += 1;
		stuRegion.itsPort = ntohs(*(unsigned short*)(buffer + pos));
		pos += 2;
		stuRegion.flag = buffer[pos];
		pos += 1;
	}

	stuRegion.siteCount = (buflen - pos + 1 + 2) / 3;
	for(int i = 0; i < stuRegion.siteCount; i++)
	{
		stuRegion.site[i].id = ntohs(*(unsigned short*)(buffer + pos));
		pos += 2;
		stuRegion.site[i].subsystemId = buffer[pos];
		pos += 1;
		stuRegion.site[i].spare = buffer[pos];
		pos += 1;
	}
	return true;
}

/* *
 * 
 * */
bool ProtocolScats::DecodeDatetime(char *buffer, T_Datetime &stuTime)
{
	if(buffer == NULL)
		return false;
	int buflen = ntohs(*((unsigned short*)buffer));
	if((BYTE)buffer[2] != 3)
		return false;

	stuTime.year = ntohs(*((unsigned short*)(buffer+3)));
	stuTime.month = buffer[5];
	stuTime.day = buffer[6];
	stuTime.hour = buffer[7];
	stuTime.minute = buffer[8];
	stuTime.second = buffer[9];
	return true;
}

/* *
 * 
 * */
bool ProtocolScats::DecodeControllerMemory(char *buffer, T_Controller &stuController)
{
	if(buffer == NULL)
		return false;
	int buflen = ntohs(*((unsigned short*)buffer));
	if((BYTE)buffer[2] != 4)
		return false;

	stuController.type = 1;
	stuController.siteId = ntohs(*(unsigned short*)(buffer + 4));
	stuController.page = buffer[6];
	if((BYTE)buffer[6] <= 15)
	{
		stuController.offset = buffer[7];
		stuController.value = buffer[8];
	}
	else if((BYTE)buffer[6] == 129)
	{
		g_logs->WriteErr("ControllerMemory: Site not found.");
		return false;
	}
	else if((BYTE)buffer[6] == 130)
	{
		g_logs->WriteErr("ControllerMemory: Controller communications error.");
		return false;
	}
	else if((BYTE)buffer[6] >= 144)
	{
		g_logs->WriteErr("ControllerMemory: Invalid page number.");
		return false;
	}
	else
	{
		g_logs->WriteErr("ControllerMemory: Can't prase.");
		return false;
	}
	return true;
}

/* *
 * 
 * */
bool ProtocolScats::DecodeControllerDetail(char *buffer, T_Controller &stuController)
{
	if(buffer == NULL)
		return false;
	int buflen = ntohs(*((unsigned short*)buffer));
	if((BYTE)buffer[2] != 4)
		return false;

	stuController.type = 2;
	stuController.siteId = ntohs(*(unsigned short*)(buffer + 4));
	if((BYTE)buffer[6] == 255)
	{
		unsigned short temp = ntohs(*(unsigned short*)(buffer+7));
		stuController.version = temp & 0x07;
		stuController.controllerType = (temp >> 3) & 0x1F;
		stuController.releaseNo = (temp >> 8) & 0xFF;
	}
	else if((BYTE)buffer[6] == 129)
	{
		g_logs->WriteErr("ControllerDetail: Site not found.");
		return false;
	}
	else if((BYTE)buffer[6] == 130)
	{
		g_logs->WriteErr("ControllerDetail: Controller communications error.");
		return false;
	}
	else
	{
		g_logs->WriteErr("ControllerDetail: Can't prase.");
		return false;
	}
	return true;
}

/* *
 * 
 * */
bool ProtocolScats::DecodeControllerExtend(char *buffer, T_ControllerExtend &stuExtend)
{
	if(buffer == NULL)
		return false;
	int buflen = ntohs(*((unsigned short*)buffer));
	if((BYTE)buffer[2] != 5)
		return false;

	stuExtend.siteId = ntohs(*(unsigned short*)(buffer + 4));
	stuExtend.lastMsgFlag = (buffer[6] >> 6) & 0x01;
	stuExtend.errorFlag = (buffer[6] >> 7) & 0x01;
	if(stuExtend.errorFlag)
	{
		if((BYTE)buffer[6] == 129)
		{
			g_logs->WriteErr("ControllerExtend: Site not found.");
		}
		else if((BYTE)buffer[6] == 130)
		{
			g_logs->WriteErr("ControllerExtend: Controller communications error.");
		}
		else
		{
			g_logs->WriteErr("ControllerExtend: Undefined Error.");
		}
	}
	stuExtend.controlId = buffer[7] & 0x7F;
	stuExtend.codeFlag = (buffer[7] >> 7) & 0x01;
	unsigned short temp = ntohs(*(unsigned short *)(buffer + 8));
	stuExtend.version = temp & 0x07;
	stuExtend.type = (temp >> 3) & 0x1F;
	stuExtend.releaseNo = (temp >> 8) & 0xFF;
	stuExtend.serialNo = buffer[10] & 0x7F;
	stuExtend.serialCodeFlag = (buffer[10] >> 7) & 0x01;
	stuExtend.serialNumber = ntohs(*(unsigned short*)(buffer + 11));
	stuExtend.versionNo = buffer[13] & 0x7F;
	stuExtend.versionCodeFlag = (buffer[13] >> 7) & 0x01;
	stuExtend.versionNumber = ntohs(*(unsigned short*)(buffer + 14));
	return true;
}

/* *
 * 
 * */
bool ProtocolScats::DecodeActionList(char *buffer, T_ActionListResponse &stuActionList)
{
	if(buffer == NULL)
		return false;
	int buflen = ntohs(*((unsigned short*)buffer));
	if((BYTE)buffer[2] != 9)
		return false;

	stuActionList.mode = buffer[3] & 0x01;
	stuActionList.status = (buffer[3] >> 4) & 0x07;

	bool result = false;
	int length = 4;
	switch(stuActionList.status)
	{
		case 0:
			result = true;break;
		case 1:
			g_logs->WriteErr("ActionList 没有发现 region/Site.");break;
		case 2:
			g_logs->WriteErr("ActionList 无效的 Action List Number.");break;
		case 3:
			g_logs->WriteErr("ActionList 读文件错误");break;
		case 4:
			g_logs->WriteErr("ActionList 没有发现 Action List Number.");break;
		case 5:
			g_logs->WriteErr("ActionList 在 Action List 命令错误.");break;
		default:
			g_logs->WriteErr("ActionList undefined errors..");break;
	}
	if(stuActionList.mode == 0)
	{
		memcpy(stuActionList.regionname, buffer+4, 6);
		length += 6;
	}
	else
	{
		stuActionList.siteId = ntohs(*((unsigned short*)(buffer+4)));
		length += 2;
	}

	stuActionList.listCount = (buflen - length + 2) / 2;
	for(int i = 0; i < stuActionList.listCount; i++)
	{
		stuActionList.list[i] = ntohs(*(unsigned short*)(buffer + length));
		length += 2;
	}
	return true;
}

/* *
 * 
 * */
bool ProtocolScats::DecodeGreenWindow(char *buffer, T_GreenWindowResponse &stuGreen)
{
	if(buffer == NULL)
		return false;
	int buflen = ntohs(*((unsigned short*)buffer));
	if((BYTE)buffer[2] != 10)
		return false;

	stuGreen.count = (buflen - 3) / 3;
	int length = 5;
	for(int i = 0; i < stuGreen.count; i++)
	{
		stuGreen.data[i].siteId = ntohs(*(unsigned short*)(buffer + length));
		length += 2;
		stuGreen.data[i].status = buffer[length];
		length += 1;
	}
	return true;
}

/* *
 * 报警命令的应答
 * */
bool ProtocolScats::DecodeAlarmResponse(char *buffer, T_AlarmResponse &stuAlarm)
{
	if(buffer == NULL)
		return false;
	int buflen = ntohs(*((unsigned short*)buffer));
	if((BYTE)buffer[2] != 12)
		return false;

	int length = 3;
	stuAlarm.flags.firstRecord = buffer[length] & 0x01;
	stuAlarm.flags.lastRecord = (buffer[length] >> 1) & 0x01;
	stuAlarm.flags.updateType = (buffer[length] >> 2) & 0x01;
	stuAlarm.flags.isAreaName = (buffer[length] >> 4) & 0x01;
	length += 1;
	if(stuAlarm.flags.isAreaName)
	{
		memcpy(stuAlarm.areaName, buffer + length, 100);
		length += 100;
	}

	if(stuAlarm.flags.isAreaName)
	{
		for(int i = 0; i < stuAlarm.datasCount; i++)
		{
			stuAlarm.datas[i].length = buffer[length];
			length += 1;
			stuAlarm.datas[i].identifier = ntohl(*(unsigned int*)(buffer + length));
			length += 4;
			unsigned short date = ntohs(*(unsigned short*)(buffer + length));
			stuAlarm.datas[i].datetime.year = ((date >> 9) & 0x3F) + 1970;
			stuAlarm.datas[i].datetime.month = (date >> 5) & 0x0F;
			stuAlarm.datas[i].datetime.day = date & 0x1F;
			length += 2;
			unsigned short time = ntohs(*(unsigned short*)(buffer + length));
			stuAlarm.datas[i].datetime.hour = (time >> 11) & 0x1F;
			stuAlarm.datas[i].datetime.minute = (time >> 5) & 0x3F;
			stuAlarm.datas[i].datetime.second = (time & 0x0F) * 2;
			length += 2;
			unsigned short ackDate = ntohs(*(unsigned short*)(buffer + length));
			stuAlarm.datas[i].ackDatetime.year = ((date >> 9) & 0x3F) + 1970;
			stuAlarm.datas[i].ackDatetime.month = (date >> 5) & 0x0F;
			stuAlarm.datas[i].ackDatetime.day = date & 0x1F;
			length += 2;
			unsigned short ackTime = ntohs(*(unsigned short*)(buffer + length));
			stuAlarm.datas[i].ackDatetime.hour = (time >> 11) & 0x1F;
			stuAlarm.datas[i].ackDatetime.minute = (time >> 5) & 0x3F;
			stuAlarm.datas[i].ackDatetime.second = (time & 0x0F) * 2;
			length += 2;
			stuAlarm.datas[i].flags.status = buffer[length] & 0x01;
			stuAlarm.datas[i].flags.ackStatus = (buffer[length] >> 1) & 0x01;
			stuAlarm.datas[i].flags.asFlag = (buffer[length] >> 2) & 0x01;
			stuAlarm.datas[i].flags.hideStatus = (buffer[length] >> 3) & 0x01;
			stuAlarm.datas[i].flags.clearFlag = (buffer[length] >> 4) & 0x01;
			stuAlarm.datas[i].flags.testFlag = (buffer[length] >> 6) & 0x01;
			length += 1;
			stuAlarm.datas[i].count = buffer[length];
			length += 1;
			stuAlarm.datas[i].ackUserId = buffer[length];
			length += 1;
			stuAlarm.datas[i].areaNameId = buffer[length];
			length += 1;
			memcpy(stuAlarm.datas[i].remark, buffer + length, stuAlarm.datas[i].length - 17);
			length += stuAlarm.datas[i].length - 17;
		}
	}
	return true;
}

/* *
 * 
 * */
bool ProtocolScats::DecodeRampMetering(char *buffer, T_DetectorData &stuDetectData)
{
	if(buffer == NULL)
		return false;
	int buflen = ntohs(*((unsigned short*)buffer));
	if((BYTE)buffer[2] != 13)
		return false;

	if(buffer[3] == 1)
	{
		stuDetectData.subtype = 1;
		int length = 3;
		int n = 0;
		while(buflen > length)
		{
			stuDetectData.stationCount += 1;
			unsigned int temp = ntohl(*(unsigned long*)(buffer + length));
			length += 4;
			stuDetectData.station[n].category = temp & 0x07;
			stuDetectData.station[n].rampNumber = (temp >> 3) & 0x03;
			stuDetectData.station[n].siteId = (temp >> 8) & 0xFFFF;
			stuDetectData.station[n].detectorCount = buffer[length];
			length += 1;
			for(int i = 0; i < stuDetectData.station[n].detectorCount; i++)
			{
				unsigned short tmp = ntohs(*(unsigned short*)(buffer + length));
				length += 2;
				stuDetectData.station[n].detector[i].occupancy = tmp & 0x03FF;
				stuDetectData.station[n].detector[i].count = (tmp >> 10) & 0x3F;
			}
		}
		stuDetectData.stationCount = n;
	}
	else if(buffer[3] == 2)
	{
		stuDetectData.subtype = 2;
	}

	return true;
}

/* *
 * 
 * */
bool ProtocolScats::DecodeBlockData(char *buffer, T_BlockData &stuBlockData)
{
	if(buffer == NULL)
		return false;
	int buflen = ntohs(*((unsigned short*)buffer));
	if((BYTE)buffer[2] != 14)
		return false;

	int length = 4;
	if(BYTE(buffer[3]) == 1)
	{
		stuBlockData.dataType = 1;
		stuBlockData.noCongestionCount = buffer[length];
		length += 1;
		for(int i = 0; i < stuBlockData.noCongestionCount; i++)
		{
			stuBlockData.siteId[i] = ntohs(*(unsigned short*)(buffer + length));
			length += 2;
		}
		stuBlockData.congestionCount = buffer[length];
		length += 1;
		for(int i = 0; i < stuBlockData.congestionCount; i++)
		{
			stuBlockData.detects[i].id = ntohs(*(unsigned short*)(buffer + length));
			length += 2;
			stuBlockData.detects[i].detectors = ntohl(*(unsigned long*)(buffer + length));
			length += 4;
		}
	}
	else if(BYTE(buffer[3]) == 2)
	{
		stuBlockData.dataType = 2;
		stuBlockData.indexCount = (buflen - length + 2) / 3;
		for(int i = 0; i < stuBlockData.indexCount; i++)
		{
			stuBlockData.indexs[i].id = ntohs(*(unsigned short*)(buffer + length));
			length += 2;
			stuBlockData.indexs[i].index = buffer[length];
			length += 1;
		}
	}
	else if(BYTE(buffer[3]) == 3)
	{
		stuBlockData.dataType = 3;
		stuBlockData.siteCount = (buflen - length + 2) / 2;
		for(int i = 0; i < stuBlockData.siteCount; i++)
		{
			stuBlockData.siteId[i] = ntohs(*(unsigned short*)(buffer + length));
			length += 2;
		}
	}
	return true;
}

/* *
 * 
 * */
bool ProtocolScats::DecodeRouteResponse(char *buffer, T_RouteResponse &stuRoute)
{
	if(buffer == NULL)
		return false;
	int buflen = ntohs(*((unsigned short*)buffer));
	if((BYTE)buffer[2] != 15)
		return false;
	bool result = false;


	stuRoute.workstation = buffer[4];
	stuRoute.flag = buffer[5];
	stuRoute.code.errorFlag = (buffer[6] >> 7) & 0x01;
	if(stuRoute.code.errorFlag)
	{
		stuRoute.code.error = buffer[6];
	}
	else
	{
		stuRoute.code.succeed  = buffer[6] & 0x01;
		stuRoute.code.allRoute  = (buffer[6] >> 1) & 0x01;
		stuRoute.code.loadedFlag  = (buffer[6] >> 3) & 0x01;
		stuRoute.code.description  = (buffer[6] >> 6) & 0x01;
	}
	int length = 7;
	if(stuRoute.code.loadedFlag)
	{
		stuRoute.loadRoute = buffer[length];
		length += 1;
	}
	stuRoute.notUse1 =  ntohs(*(unsigned short*)(buffer + length));
	length += 2;
	stuRoute.notUse2 =  ntohs(*(unsigned short*)(buffer + length));
	length += 2;

	stuRoute.routeCount = 0;			// ???????????????
	for(int i = 0; i < stuRoute.routeCount; i++)
	{
		unsigned short temp = ntohs(*(unsigned short*)(buffer + length));
		length += 2;
		stuRoute.route[i].active = (temp >> 15) & 0x01;
		stuRoute.route[i].routeNum = temp & 0x7FFF;
	}
	if(stuRoute.code.description)
	{	// 包含此字段时则没有后续的siteData数据
		int len = buflen + 2 - length - 4;
		memcpy(stuRoute.description, buffer + length, len);
		length += len;
	}

	if(!stuRoute.code.loadedFlag && !stuRoute.code.description)
	{
		stuRoute.siteNumber = buffer[length];
		length += 1;
	}
	unsigned short temp = ntohs(*(unsigned short*)(buffer + length));
	length += 2;
	stuRoute.flags.cancel = (temp >> 4) & 0x01;
	stuRoute.flags.held = (temp >> 7) & 0x01;
	stuRoute.flags.privilege = (temp >> 14) & 0x01;
	stuRoute.flags.active = (temp >> 15) & 0x01;
	stuRoute.routeNumber = ntohs(*(unsigned short*)(buffer + length));
	length += 2;

	for(int i = 0; i < stuRoute.siteNumber; i++)
	{
		;
	}
	return result;
}

/* *
 * 
 * */
bool ProtocolScats::DecodeANTTSResponse(char *buffer, T_ANTTSResponse &stuANTTS)
{
	if(buffer == NULL)
		return false;
	int buflen = ntohs(*((unsigned short*)buffer));
	if((BYTE)buffer[2] != 16)
		return false;
	bool result = false;

	return result;
}

/* *
 * 
 * */
bool ProtocolScats::DecodeLockPhaseOrder(char *buffer)
{
	if(buffer == NULL)
		return false;
	int buflen = ntohs(*((unsigned short*)buffer));
	if((BYTE)buffer[2] != 30)
		return false;

	bool result = false;
	switch((BYTE)buffer[4])
	{
		case 0:
			result = true;break;
		case 1:
			g_logs->WriteLog("LockPhaseOrder: No this site.");break;
		case 2:
			g_logs->WriteLog("LockPhaseOrder: Invalid phase.");break;
		case 3:
			g_logs->WriteLog("LockPhaseOrder: No access.");break;
		case 4:
			g_logs->WriteLog("LockPhaseOrder: Invalid lock time.");break;
		default:
			g_logs->WriteLog("LockPhaseOrder: Cannot identify the identifier.");
	}
	return result;
}

/* *
 * 
 * */
bool ProtocolScats::DecodeIntervene(char *buffer)
{
	if(buffer == NULL)
		return false;
	int buflen = ntohs(*((unsigned short*)buffer));
	if((BYTE)buffer[2] != 31)
		return false;

	bool result = false;
	switch((BYTE)buffer[5])
	{
		case 0:
			g_logs->WriteLog("Intervene Succeed!");
			result = true;break;
		case 1:
			g_logs->WriteErr("Intervene: Region not found!");break;
		case 2:
			g_logs->WriteErr("Intervene: Subsystem number out of range!");break;
		case 3:
			g_logs->WriteErr("Intervene: Site ID not found!");break;
		case 4:
			g_logs->WriteErr("Intervene: User no access!");break;
		case 5:
			g_logs->WriteErr("IIntervene: Illegal value!");break;
	}
	return result;
}

/* *
 * 
 * */
bool ProtocolScats::DecodeLaneStatus(char *buffer, T_LaneStatusResponse &stuLaneStatus)
{
	if(buffer == NULL)
		return false;
	int buflen = ntohs(*((unsigned short*)buffer));
	if((BYTE)buffer[2] != 32)
		return false;

	stuLaneStatus.second = ntohs(*(unsigned short*)(buffer + 3));

	int length = 5;
	if(stuLaneStatus.second == 65535)
	{
		if(buflen <= 3)
			return true;

		stuLaneStatus.dataType = 1;
		while(true)
		{
			if((buflen + 2) > length)
			{
				switch(BYTE(buffer[length]))
				{
					case 1:
						length += 1;
						stuLaneStatus.unknownSiteCount = buffer[length];
						length += 1;
						for(int i = 0; i < stuLaneStatus.unknownSiteCount; i++)
						{
							stuLaneStatus.unknownSiteIds[i] = (*(unsigned short*)(buffer + length));
							length += 2;
						}
						break;
					case 2:
						length += 1;
						stuLaneStatus.inaccessSiteCount = buffer[length];
						length += 1;
						for(int i = 0; i < stuLaneStatus.inaccessSiteCount; i++)
						{
							stuLaneStatus.inaccessSiteIds[i] = ntohs(*(unsigned short*)(buffer + length));
							length += 2;
						}
						break;
					default:
						;
				}
			}
			else
			{
				break;
			}
		}
	}
	else if(stuLaneStatus.second == 65534)
	{
		stuLaneStatus.dataType = 2;
		stuLaneStatus.phaseCount = (buflen - 3) / 4;
		for(int i = 0; i < stuLaneStatus.phaseCount; i++)
		{
			stuLaneStatus.phaseInfo[i].siteid = ntohs(*(unsigned short*)(buffer + length));
			length += 2;
			stuLaneStatus.phaseInfo[i].phaseId = buffer[length] & 0x07;
			stuLaneStatus.phaseInfo[i].phaseStat = (buffer[length] >> 3) & 0x07;
			stuLaneStatus.phaseInfo[i].change = (buffer[length] >> 6) & 0x01;
			stuLaneStatus.phaseInfo[i].delay = (buffer[length] >> 7) & 0x01;
			length += 1;
			stuLaneStatus.phaseInfo[i].lampStatus = buffer[length] & 0x03;
			length += 1;
		}
	}
	else
	{
		stuLaneStatus.dataType = 3;
		int i = 0;
		while(buflen > length - 2)
		{
			stuLaneStatus.phaseData[i].siteid = ntohs(*(unsigned short*)(buffer + length));
			length += 2;
			stuLaneStatus.phaseData[i].flags.bSS = buffer[length] & 0x01;
			stuLaneStatus.phaseData[i].flags.bPSL = (buffer[length] >> 1) & 0x01;
			stuLaneStatus.phaseData[i].flags.bPPP = (buffer[length] >> 2) & 0x01;
			stuLaneStatus.phaseData[i].flags.bFC = (buffer[length] >> 3) & 0x01;
			stuLaneStatus.phaseData[i].flags.bSP = (buffer[length] >> 4) & 0x01;
			stuLaneStatus.phaseData[i].flags.bSSNR = (buffer[length] >> 5) & 0x01;
			length += 1;
			if(stuLaneStatus.phaseData[i].flags.bSS)
			{
				unsigned short temp = ntohs(*(unsigned short*)(buffer + length));
				stuLaneStatus.phaseData[i].siteStatus.bSIDA = (temp >> 0) & 0x01;
				stuLaneStatus.phaseData[i].siteStatus.bFYBO = (temp >> 1) & 0x01;
				stuLaneStatus.phaseData[i].siteStatus.bWAlarm = (temp >> 2) & 0x01;
				stuLaneStatus.phaseData[i].siteStatus.bMAlarm = (temp >> 3) & 0x01;
				stuLaneStatus.phaseData[i].siteStatus.bHpMode = (temp >> 4) & 0x01;
				stuLaneStatus.phaseData[i].siteStatus.bFallback = (temp >> 5) & 0x01;
				stuLaneStatus.phaseData[i].siteStatus.bClloss = (temp >> 6) & 0x01;
				stuLaneStatus.phaseData[i].siteStatus.bDwell = (temp >> 7) & 0x01;
				stuLaneStatus.phaseData[i].siteStatus.bAlarmDetail = (temp >> 8) & 0x01;
				stuLaneStatus.phaseData[i].siteStatus.byPlanStat = (temp >> 11) & 0x03;
				stuLaneStatus.phaseData[i].siteStatus.bCongested = (temp >> 13) & 0x01;
				stuLaneStatus.phaseData[i].siteStatus.bySlMode = (temp >> 14) & 0x03;
				length += 2;
			}

			if(stuLaneStatus.phaseData[i].siteStatus.bAlarmDetail)
			{
				stuLaneStatus.phaseData[i].alarmFlags = ntohl(*(unsigned int*)(buffer + length));
				length += 4;
			}
			if(false)
			{
				stuLaneStatus.phaseData[i].detectorFlags = ntohl(*(unsigned int*)(buffer + length));
				length += 4;
			}

			if(stuLaneStatus.phaseData[i].flags.bPSL)
			{
				stuLaneStatus.phaseData[i].planNo.bySplitPlanNo = buffer[length] & 0x1F;
				stuLaneStatus.phaseData[i].planNo.byOffsetPlanNo = (buffer[length] >> 5) & 0x07;
				length += 1;
				stuLaneStatus.phaseData[i].specialFac.bY1 = buffer[length] & 0x01;
				stuLaneStatus.phaseData[i].specialFac.bY2 = (buffer[length] >> 1) & 0x01;
				stuLaneStatus.phaseData[i].specialFac.bZ1 = (buffer[length] >> 2) & 0x01;	
				stuLaneStatus.phaseData[i].specialFac.bZ2 = (buffer[length] >> 3) & 0x01;	
				stuLaneStatus.phaseData[i].specialFac.byCtrlMode = (buffer[length] >> 5) & 0x07;	
				length += 1;
				stuLaneStatus.phaseData[i].locks.bSplit = buffer[length] & 0x01;
				stuLaneStatus.phaseData[i].locks.bOffset = (buffer[length] >> 1) & 0x01;
				stuLaneStatus.phaseData[i].locks.bSysFlag = (buffer[length] >> 7) & 0x01;
				length += 1;
			}
			if(stuLaneStatus.phaseData[i].flags.bPPP)
			{
				unsigned short temp = ntohs(*(unsigned short*)(buffer + length));
				stuLaneStatus.phaseData[i].data.phaseTime = temp & 0x01FF;
				stuLaneStatus.phaseData[i].data.phaseInterval = (temp >> 10) & 0x07;
				stuLaneStatus.phaseData[i].data.phaseId = (temp >> 13) & 0x07;
				length += 2;
				stuLaneStatus.phaseData[i].time = ntohs(*(unsigned short*)(buffer + length));
				length += 2;
				stuLaneStatus.phaseData[i].status = ntohs(*(unsigned short*)(buffer + length));
				length += 2;
			}
			if(stuLaneStatus.phaseData[i].flags.bFC)
			{
				stuLaneStatus.phaseData[i].flagsContent = buffer[length];
				length += 1;
				if(stuLaneStatus.phaseData[i].flagsContent & 0x01)
				{
					stuLaneStatus.phaseData[i].XSF_flags = ntohs(*(unsigned short*)(buffer + length));
					length += 2;
				}
				if((stuLaneStatus.phaseData[i].flagsContent >> 1) & 0x01)
				{
					stuLaneStatus.phaseData[i].MSS_flags = ntohs(*(unsigned short*)(buffer + length));
					length += 2;
				}	
				if((stuLaneStatus.phaseData[i].flagsContent >> 2) & 0x01)
				{
					stuLaneStatus.phaseData[i].RSF_flags = ntohs(*(unsigned short*)(buffer + length));
					length += 2;
				}
				stuLaneStatus.phaseData[i].zFlags = buffer[length];	
				length += 1;
			}
			if(stuLaneStatus.phaseData[i].flags.bSP)
			{
				stuLaneStatus.phaseData[i].signalGroupData = ntohs(*(unsigned short*)(buffer + length));
				length += 4;
				stuLaneStatus.phaseData[i].phaseDemands = buffer[length];	
				length += 1;
			}
			if(stuLaneStatus.phaseData[i].flags.bSSNR)
			{
				stuLaneStatus.phaseData[i].bySubsystemNo = buffer[length];	
				length += 1;
				stuLaneStatus.phaseData[i].stSubSysflag.byCongIndex = buffer[length] & 0x0F;
				stuLaneStatus.phaseData[i].stSubSysflag.bMarriage = (buffer[length] >> 4) & 0x01;
				length += 1;
				stuLaneStatus.phaseData[i].nominalCycle = ntohs(*(unsigned short*)(buffer + length));
				length += 2;
				stuLaneStatus.phaseData[i].requireCycle = ntohs(*(unsigned short*)(buffer + length));
				length += 2;
			}
			stuLaneStatus.dataCount += 1;
			i += 1;
		}
	}
	return true;
}

/* *
 * 
 * */
bool ProtocolScats::DecodeStrategyMonitor(char *buffer)
{
	if(buffer == NULL)
		return false;
	int buflen = ntohs(*((unsigned short*)buffer));
	if((BYTE)buffer[2] != 33)
		return false;

	bool result = false;
	if((buffer[3] & 0x01) == 1)
	{
		g_logs->WriteLog("StrategyMonitor: The area was not found.");
	}
	else
	{
		g_logs->WriteLog("StrategyMonitor: Set strategy monitor succeed.");
		result = true;
	}
	return result;
}

/* *
 * 
 * */
bool ProtocolScats::DecodeDetectorCount(char *buffer, T_DetectorCountResponse stuDetectCount)
{
	if(buffer == NULL)
		return false;
	int buflen = ntohs(*((unsigned short*)buffer));
	if((BYTE)buffer[2] != 35)
		return false;

	stuDetectCount.period = buffer[3] & 0x01;
	unsigned short ymd = ntohs(*(unsigned short*)(buffer + 4));
	unsigned short hms = ntohs(*(unsigned short*)(buffer + 6));
	stuDetectCount.time.year = ((ymd >> 9) & 0x7F) + 1970;
	stuDetectCount.time.month = (ymd >> 4) & 0x0F;
	stuDetectCount.time.day = ymd & 0x1F;
	stuDetectCount.time.hour = (hms >> 11) & 0x1F;
	stuDetectCount.time.minute = (hms >> 6) & 0x3F;
	stuDetectCount.time.second = hms & 0x1F;
	int length = 8;

	int count = 0;
	while((buflen + 2) > length)
	{
		stuDetectCount.siteData[count].siteId = ntohs(*(unsigned short*)(buffer + length));
		length += 2;
		stuDetectCount.siteData[count].detectCount = buffer[length];
		length += 1;
		for(int i = 0; i < stuDetectCount.siteData[count].detectCount; i++)
		{
			stuDetectCount.siteData[count].detector[i].id = buffer[length];
			length += 1;
			stuDetectCount.siteData[count].detector[i].data = ntohs(*(unsigned short*)(buffer + length));
			length += 2;
		}
	}
	stuDetectCount.count += count;
	return true;
}

/* *
 * 
 * */
bool ProtocolScats::DecodeRegisterService(char *buffer, T_RegisterService &stuRegService)
{
	if(buffer == NULL)
		return false;
	int buflen = ntohs(*((unsigned short*)buffer));
	if((BYTE)buffer[2] != 150)
		return false;

	stuRegService.dataId1 = buffer[3];
	stuRegService.dataId2 = buffer[4];
	stuRegService.dataId3 = buffer[5];
	stuRegService.dataId4 = buffer[6];
	return true;
}

/* *
 * 解析错误应答消息,当发送的消息错误时，scats会返回此类型消息
 * */
bool ProtocolScats::DecodeErrorResponse(char *buffer)
{
	if(buffer == NULL)
		return false;
	if((BYTE)buffer[2] != 255)
		return false;

	switch(BYTE(buffer[4]))
	{
		case 1:
			g_logs->WriteLog("ErrorResponse: Unknown message type.");break;
		case 2:
			g_logs->WriteLog("ErrorResponse: Unlicensed message request.");break;
		case 3:
			g_logs->WriteLog("ErrorResponse: Server not found.");break;
		case 4:
			g_logs->WriteLog("ErrorResponse: Client not found.");break;
		default:
			g_logs->WriteLog("ErrorResponse: Can't parse error message.");
	}
	return true;
}

/* ****************************************************************************************************************************************** */

void ProtocolScats::SetLaneStatusRequest(const T_LaneStatusRequest &data)
{
	pthread_mutex_lock(&m_mutexLSR);
	memcpy(m_stuLaneStatusRequest, &data, sizeof(T_LaneStatusRequest));
	pthread_mutex_unlock(&m_mutexLSR);
}

/* ****************************************************************************************************************************************** */

void VoidPtr::SetPtr(int n)
{
	switch(n)
	{
		case 2:
		case 22:
			type = 2;
			data = new T_RegionDetail;
			memset(data, 0, sizeof(T_RegionDetail));
			break;
		case 3:
			type = 3;
			data = new T_Datetime;
			memset(data, 0, sizeof(T_Datetime));
			break;
		case 4:
			type = 4;
			data = new T_Controller;
			memset(data, 0, sizeof(T_Controller));
			break;
		case 5:
			type = 5;
			data = new T_ControllerExtend;
			memset(data, 0, sizeof(T_ControllerExtend));
			break;
		case 9:
			type = 9;
			data = new T_ActionListResponse;
			memset(data, 0, sizeof(T_ActionListResponse));
			break;
		case 10:
			type = 10;
			data = new T_GreenWindowResponse;
			memset(data, 0, sizeof(T_GreenWindowResponse));
			break;
		case 12:
			type = 12;
			data = new T_AlarmResponse;
			memset(data, 0, sizeof(T_AlarmResponse));
			break;
		case 13:
			type = 13;
			data = new T_DetectorData;
			memset(data, 0, sizeof(T_DetectorData));
			break;
		case 14:
			type = 14;
			data = new T_BlockData;
			memset(data, 0, sizeof(T_BlockData));
			break;
		case 15:
			type = 15;
			data = new T_RouteResponse;
			memset(data, 0, sizeof(T_RouteResponse));
			break;
		case 16:
			type = 16;
			data = new T_ANTTSResponse;
			memset(data, 0, sizeof(T_ANTTSResponse));
			break;
		case 32:
			type = 32;
			data = new T_LaneStatusResponse;
			memset(data, 0, sizeof(T_LaneStatusResponse));
			break;
		case 35:
			type = 35;
			data = new T_DetectorCountResponse;
			memset(data, 0, sizeof(T_DetectorCountResponse));
			break;
		case 150:
			type = 150;
			data = new T_RegisterService;
			memset(data, 0, sizeof(T_RegisterService));
			break;
		default:
			break;
	}
}

void VoidPtr::DelPtr()
{
	switch(type)
	{
		case 2:
			if(data != NULL)
			{
				delete (T_RegionDetail*)data;
				data = NULL;
			}
			break;
		case 3:
			if(data != NULL)
			{
				delete (T_Datetime*)data;
				data = NULL;
			}
			break;
		case 4:
			if(data != NULL)
			{
				delete (T_Controller*)data;
				data = NULL;
			}
			break;
		case 5:
			if(data != NULL)
			{
				delete (T_ControllerExtend*)data;
				data = NULL;
			}
			break;
		case 9:
			if(data != NULL)
			{
				delete (T_ActionListResponse*)data;
				data = NULL;
			}
			break;
		case 10:
			if(data != NULL)
			{
				delete (T_GreenWindowResponse*)data;
				data = NULL;
			}
			break;
		case 12:
			if(data != NULL)
			{
				delete (T_AlarmResponse*)data;
				data = NULL;
			}
			break;
		case 13:
			if(data != NULL)
			{
				delete (T_DetectorData*)data;
				data = NULL;
			}
			break;
		case 14:
			if(data != NULL)
			{
				delete (T_BlockData*)data;
				data = NULL;
			}
			break;
		case 15:
			if(data != NULL)
			{
				delete (T_RouteResponse*)data;
				data = NULL;
			}
			break;
		case 16:
			if(data != NULL)
			{
				delete (T_ANTTSResponse*)data;
				data = NULL;
			}
			break;
		case 32:
			if(data != NULL)
			{
				delete (T_LaneStatusResponse*)data;
				data = NULL;
			}
			break;
		case 35:
			if(data != NULL)
			{
				delete (T_DetectorCountResponse*)data;
				data = NULL;
			}
			break;
		case 150:
			if(data != NULL)
			{
				delete (T_RegisterService*)data;
				data = NULL;
			}
			break;
	}
	type = 0;
}

/* ****************************************************************************************************************************************** */

void ProtocolScats::print_msg(BYTE type, void *ptrData)
{
	switch(type)
	{
		case 2:
		case 22:
			cout << "Message  2:" << endl;
			cout << "RegionNo  : " << ((T_RegionDetail*)ptrData)->regionNo << endl;
			cout << "version   : " << (int)((T_RegionDetail*)ptrData)->version.main << "." << (int)((T_RegionDetail*)ptrData)->version.major << "." 
				 << (int)((T_RegionDetail*)ptrData)->version.minor << endl;
			cout << "RegionName: " << ((T_RegionDetail*)ptrData)->regionName << endl;
			cout << "Support   : ";
			if(((T_RegionDetail*)ptrData)->support.isFixedTime)
			{
				cout << " FixedTime";
			}
			if(((T_RegionDetail*)ptrData)->support.isAdaptive)
			{
				cout << " Adaptive";
			}
			if(((T_RegionDetail*)ptrData)->support.isCMS)
			{
				cout << " CMS";
			}
			if(((T_RegionDetail*)ptrData)->support.isDIDO)
			{
				cout << " DIDO";
			}
			if(((T_RegionDetail*)ptrData)->support.isANTTS)
			{
				cout << " ANIIT";
			}
			cout << endl;
			cout << "Ipaddr   : " << ((T_RegionDetail*)ptrData)->ipaddr << endl;
			cout << "Port     : " << ((T_RegionDetail*)ptrData)->port << endl;
			cout << "Count    : " << ((T_RegionDetail*)ptrData)->count << endl;
			cout << "BuildNo  : " << ((T_RegionDetail*)ptrData)->buildNo << endl;
			cout << "Itsport  : " << ((T_RegionDetail*)ptrData)->itsPort << endl;
			cout << "Flag     : " << ((T_RegionDetail*)ptrData)->flag << endl;
			for(int i = 0; i < ((T_RegionDetail*)ptrData)->siteCount; i++)
			{
				cout << "Site[" << i << "] Id   : " << ((T_RegionDetail*)ptrData)->site[i].id << endl;
				cout << "Site[" << i << "] SubId: " << ((T_RegionDetail*)ptrData)->site[i].subsystemId << endl;
			}
			break;
		case 3:
			cout << "Message 3: " << endl;
			cout << "year  : " << (int)((T_Datetime*)ptrData)->year << endl;;
			cout << "month : " << (int)((T_Datetime*)ptrData)->month << endl;
			cout << "day   : " << (int)((T_Datetime*)ptrData)->day << endl;
			cout << "hour  : " << (int)((T_Datetime*)ptrData)->hour << endl;
			cout << "minute: " << (int)((T_Datetime*)ptrData)->minute << endl;
			cout << "second: " << (int)((T_Datetime*)ptrData)->second << endl;
			break;
		case 4:
			cout << "Message  4: " << endl;
			cout << "Type      : " << (int)((T_Controller*)ptrData)->type << endl;
			cout << "SiteId    : " << (int)((T_Controller*)ptrData)->siteId << endl;
			cout << "Page      : " << (int)((T_Controller*)ptrData)->page << endl;
			cout << "Offset    : " << (int)((T_Controller*)ptrData)->offset << endl;
			cout << "Value     : " << (int)((T_Controller*)ptrData)->value << endl;
			cout << "Version   : " << (int)((T_Controller*)ptrData)->version << endl;
			cout << "CtrlType  : " << (int)((T_Controller*)ptrData)->controllerType << endl;
			cout << "ReleaseNo : " << (int)((T_Controller*)ptrData)->releaseNo << endl;
			break;
		case 5:
			cout << "Message  5:" << endl;
			cout << (int)((T_ControllerExtend*)ptrData)->siteId << endl;
			cout << (int)((T_ControllerExtend*)ptrData)->lastMsgFlag << endl;
			break;
		case 9:
			cout << "Message  9:" << endl;
			cout << "Mode      : " << (int)((T_ActionListResponse*)ptrData)->mode << endl;
			cout << "Status    : " << (int)((T_ActionListResponse*)ptrData)->status << endl;
			cout << "RegionName: " << ((T_ActionListResponse*)ptrData)->regionname << endl;
			cout << "SiteId    : " << (int)((T_ActionListResponse*)ptrData)->siteId << endl;
			for(int i = 0; i < (int)((T_ActionListResponse*)ptrData)->listCount; i++)
			{
				cout << "List      : " << (int)((T_ActionListResponse*)ptrData)->list[i] << endl;
			}
			break;
		case 10:
			cout << "Message 10:" << endl;
			for(int i = 0; i < (int)((T_GreenWindowResponse*)ptrData)->count; i++)
			{
				cout << "SiteId    : " << (int)((T_GreenWindowResponse*)ptrData)->data[i].siteId << endl;
				cout << "Status    : " << (int)((T_GreenWindowResponse*)ptrData)->data[i].status << endl;
			}
			break;
		case 13:
			cout << "Message 13:" << endl;
			cout << "SubType   : " << (int)((T_DetectorData*)ptrData)->stationCount << endl;
			for(int i = 0; i < (int)((T_DetectorData*)ptrData)->stationCount; i++)
			{
				cout << "StationId  :" << (int)((T_DetectorData*)ptrData)->station[i].stationid << endl;
				cout << "Category   :" << (int)((T_DetectorData*)ptrData)->station[i].category << endl;
				cout << "RampNumber :" << (int)((T_DetectorData*)ptrData)->station[i].rampNumber << endl;
				cout << "SiteId     :" << (int)((T_DetectorData*)ptrData)->station[i].siteId << endl;
				for(int j = 0; (int)((T_DetectorData*)ptrData)->station[i].detectorCount; j++)
				{
					cout << "Occupancy: " << (int)((T_DetectorData*)ptrData)->station[i].detector[j].occupancy << endl;
					cout << "Count    : " << (int)((T_DetectorData*)ptrData)->station[i].detector[j].count << endl;
				}
			}
			break;
		case 14:
			cout << "Message 14:" << endl;
			if((int)((T_BlockData*)ptrData)->dataType == 1)
			{
				for(int i = 0; (int)((T_BlockData*)ptrData)->noCongestionCount; i++)
				{
					cout << "SiteId :" << (int)((T_BlockData*)ptrData)->siteId[i] << endl;
				}
				for(int i = 0; (int)((T_BlockData*)ptrData)->congestionCount; i++)
				{
					cout << "Id       :" << (int)((T_BlockData*)ptrData)->detects[i].id << endl;
					cout << "Detector :" << (int)((T_BlockData*)ptrData)->detects[i].detectors << endl;
				}
			}
			if((int)((T_BlockData*)ptrData)->dataType == 2)
			{
				for(int i = 0; (int)((T_BlockData*)ptrData)->indexCount; i++)
				{
					cout << "Id    :" << (int)((T_BlockData*)ptrData)->indexs[i].id << endl;
					cout << "Index :" << (int)((T_BlockData*)ptrData)->indexs[i].index << endl;
				}

			}
			if((int)((T_BlockData*)ptrData)->dataType == 3)
			{
				for(int i = 0; (int)((T_BlockData*)ptrData)->siteCount; i++)
				{
					cout << "SiteId :" << (int)((T_BlockData*)ptrData)->siteId[i] << endl;
				}
			}
			break;
		case 32:
			if((int)((T_LaneStatusResponse*)ptrData)->dataType == 1)
			{
				cout << "[SubType1] Second: " << (int)((T_LaneStatusResponse*)ptrData)->second << endl;
				for(int i = 0; i < (int)((T_LaneStatusResponse*)ptrData)->unknownSiteCount; i++)
				{
				}
				for(int i = 0; i < (int)((T_LaneStatusResponse*)ptrData)->inaccessSiteCount; i++)
				{
				}
				cout << endl;
			}
			else if((int)((T_LaneStatusResponse*)ptrData)->dataType == 2)
			{
				cout << "[SubType2] Second: " << (int)((T_LaneStatusResponse*)ptrData)->second << endl;
				for(int i = 0; i < (int)((T_LaneStatusResponse*)ptrData)->phaseCount; i++)
				{
					cout << "SiteID : " << (int)((T_LaneStatusResponse*)ptrData)->phaseInfo[i].siteid << ", 相位编号: " << (int)((T_LaneStatusResponse*)ptrData)->phaseInfo[i].phaseId << ", 相位间隔: " << (int)((T_LaneStatusResponse*)ptrData)->phaseInfo[i].phaseStat;
					if(((T_LaneStatusResponse*)ptrData)->phaseInfo[i].change)
					{
						cout << ", 相位改变";
					}

					if(((T_LaneStatusResponse*)ptrData)->phaseInfo[i].delay)
					{
						cout << ", 相位延长";
					}
					cout << ", 状态: " << (int)((T_LaneStatusResponse*)ptrData)->phaseInfo[i].lampStatus << endl;
				}
			}
			else if((int)((T_LaneStatusResponse*)ptrData)->dataType == 3)
			{
				cout << "Type3_Second : " << (int)((T_LaneStatusResponse*)ptrData)->second << endl;
				for(int i = 0; i < (int)((T_LaneStatusResponse*)ptrData)->dataCount; i++)
				{
					cout << "站点ID      : " << (int)((T_LaneStatusResponse*)ptrData)->phaseData[i].siteid << endl;
//					cout << "Flag   : " << (int)((T_LaneStatusResponse*)ptrData)->phaseData[i].flags. << endl;
//					cout << "Status : " << (int)((T_LaneStatusResponse*)ptrData)->phaseData[i].siteStatus. << endl;
//					cout << "Alarm : " << (int)((T_LaneStatusResponse*)ptrData)->phaseData[i].alarmFlags << endl;
//					cout << "SiteId : " << (int)((T_LaneStatusResponse*)ptrData)->phaseData[i].detectorFlags << endl;
//					cout << "Planno : " << (int)((T_LaneStatusResponse*)ptrData)->phaseData[i].planNo << endl;
//					cout << "Special : " << (int)((T_LaneStatusResponse*)ptrData)->phaseData[i].specialFac << endl;
//					cout << "Locks : " << (int)((T_LaneStatusResponse*)ptrData)->phaseData[i].locks << endl;
					cout << "阶段运行时间 : " << (int)((T_LaneStatusResponse*)ptrData)->phaseData[i].data.phaseTime << endl;
					cout << "当前灯态     : " << (int)((T_LaneStatusResponse*)ptrData)->phaseData[i].data.phaseInterval << endl;
					cout << "当前相位 A-G : " << (int)((T_LaneStatusResponse*)ptrData)->phaseData[i].data.phaseId << endl;
//					cout << "Time : " << (int)((T_LaneStatusResponse*)ptrData)->phaseData[i].time << endl;
//					cout << "Status : " << (int)((T_LaneStatusResponse*)ptrData)->phaseData[i].status << endl;
//					cout << "Flags : " << (int)((T_LaneStatusResponse*)ptrData)->phaseData[i].flagsContent << endl;
//					cout << "Extra : " << (int)((T_LaneStatusResponse*)ptrData)->phaseData[i].XSF_flags << endl;
//					cout << "Misce : " << (int)((T_LaneStatusResponse*)ptrData)->phaseData[i].MSS_flags << endl;
//					cout << "Remote : " << (int)((T_LaneStatusResponse*)ptrData)->phaseData[i].RSF_flags << endl;
//					cout << "zFlag : " << (int)((T_LaneStatusResponse*)ptrData)->phaseData[i].zFlags << endl;
//					cout << "Groupdata : " << (int)((T_LaneStatusResponse*)ptrData)->phaseData[i].signalGroupData << endl;
//					cout << "Demands : " << (int)((T_LaneStatusResponse*)ptrData)->phaseData[i].phaseDemands << endl;
					cout << "SubsystemNo : " << (int)((T_LaneStatusResponse*)ptrData)->phaseData[i].bySubsystemNo << endl;
					cout << "Others1  : " << (int)((T_LaneStatusResponse*)ptrData)->phaseData[i].stSubSysflag.byCongIndex << endl;
					cout << "Others2  : " << (int)((T_LaneStatusResponse*)ptrData)->phaseData[i].stSubSysflag.bMarriage << endl;
					cout << "NomCycle : " << (int)((T_LaneStatusResponse*)ptrData)->phaseData[i].nominalCycle << endl;
					cout << "ReqCycle : " << (int)((T_LaneStatusResponse*)ptrData)->phaseData[i].requireCycle << endl;
				}
				cout << endl;
			}
			break;
		case 35:
			cout << "Message 35:" << endl;
			cout << "Period : " << (int)((T_DetectorCountResponse*)ptrData)->period << endl;
			cout << "Date   : " << (int)((T_DetectorCountResponse*)ptrData)->time.year << "-" << (int)((T_DetectorCountResponse*)ptrData)->time.month 
				 << "-" << (int)((T_DetectorCountResponse*)ptrData)->time.day << " " << (int)((T_DetectorCountResponse*)ptrData)->time.hour << ":" 
				 << (int)((T_DetectorCountResponse*)ptrData)->time.minute << ":" << (int)((T_DetectorCountResponse*)ptrData)->time.second << endl;
			for(int i = 0; i < (int)((T_DetectorCountResponse*)ptrData)->count; i++)
			{
				cout << "SiteID  : " << (int)((T_DetectorCountResponse*)ptrData)->siteData[i].siteId << endl;
				for(int j = 0; j < (int)((T_DetectorCountResponse*)ptrData)->siteData[i].detectCount; j++)
				{
					cout << "detectorId  : " << (int)((T_DetectorCountResponse*)ptrData)->siteData[i].detector[j].id << endl;
					cout << "detectorData: " << (int)((T_DetectorCountResponse*)ptrData)->siteData[i].detector[j].data << endl;
				}
			}
			break;
		case 150:
			cout << "Message 150:" << endl;
			cout << "DataId1: " << (int)((T_RegisterService*)ptrData)->dataId1 << endl;
			cout << "DataId2: " << (int)((T_RegisterService*)ptrData)->dataId2 << endl;
			cout << "DataId3: " << (int)((T_RegisterService*)ptrData)->dataId3 << endl;
			cout << "DataId4: " << (int)((T_RegisterService*)ptrData)->dataId4 << endl;
			break;
		default:
			break;
	}
}
