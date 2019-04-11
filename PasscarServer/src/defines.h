#ifndef DEFINES_H_H
#define DEFINES_H_H

#include "log.h"
#include <vector>
#include <iconv.h>

#define RESPONSE_SIZE 18
#define DSC_SIGN 	1
#define MAX_EVENTS	256				//Max clients number
#define LISTEN_NUM	5				//listen param
#define BUF_SIZE	(1024*4)		//buffer size
#define TIMEOUT		1000			//timeout
#define MAXPACKLEN 	1536			//最大包长
#define SQLLEN 		2048

#define NO_DEBUG

typedef enum{
/************************** 正常应答 ***************************************/
	normal = 0x11,					// 正常
/************************* 非正常应答 **************************************/
	crc_error = 0x20,				// CRC校验错误
	lanecount_error = 0x21,			// 车道数错误
	
/************************** 内部使用 ***************************************/
	protocol_error = 0xA0,			// 协议错误
	memory_error = 0xA1,			// 内存错误
	insert_error = 0xA2				// 数据库插入失败
}ParseResult;

typedef struct 
{	//实时包
	char devicecode[33];	//设备编号
	char stationcode[16];	//站点编号
	char checktime[20];		//数据时间

	char direction;			//方向
	char lane;				//车道号
	char carType;			//车型
	char isBeijing;			//是否京牌
	char plateNumber[16];	//车牌号
	short speed;			//车速
	char plateColor;		//车牌颜色
	char carColor;			//车身颜色
	char signType;			//车标种类
	char plateType;			//号牌种类
	short illegalNumber;	//违法编号
	short recordType;		//记录类型
	short dataSrc;			//数据来源
	char axlType;			//轴型
	long int weight;		//车重
	char picName1[37];		//图1名称
	char picPath1[9];		//图1路径
	char picName2[37];		//图2名称
	char picPath2[9];		//图2路径

	int year;
	int month;
	int mday;
	int hour;
	int minute;
	int second;
}VmsDataType1;

struct LaneData
{
	char lane;				//车道号
	short speed;			//车速
	long int volume;		//车数
};

typedef struct 
{	//统计包
	char devicecode[33];	//设备编号
	char stationcode[16];	//站点编号
	char checktime[20];		//数据时间

	char direction;			//方向
	char lanecount;			//车道数
	LaneData veh[8];		//车道数据,最多8车道
	short speed;			//车速
	long int weight;		//车重

	int year;
	int month;
	int mday;
	int hour;
	int minute;
	int second;
}VmsDataType2;

typedef struct
{
	int type;			//包类型，对应union
	int clientnum;		//客户端编号
	char onlysign[7];	//唯一标识
	char isretry;		//是否重发
	char errorcode;		//错误码
	char retrytimes;	//重发次数

	union
	{
		VmsDataType1 data1;		// type == 1
		VmsDataType2 data2;		// type == 2
	};
}VmsData;

/*日志数据*/
typedef struct
{
	char devcode[20];				//设备编号
	char stationcode[16];			//站点编号
	char createtime[32];
	char servertime[32];
	int isretried;
	int messagetype;
	int parseresult;
	int msglen;
	char remote_ip[16];
	char sourcemsg[MAXPACKLEN*2+1];
}stuLogInfo;

/* 数据库信息 */
struct DbInfo{
	char strDataConnect[256];					//数据库连接字符串
	char strLogsConnect[256];					//日志库连接字符串
};

#endif
