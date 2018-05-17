//--------------------------------------------------------------
// Time		:2016-12-14
// Author	:邵佳兴
// Effect	:包含所有常用头文件，在每个文件内引用
//--------------------------------------------------------------

#ifndef _DEFINES_H_H
#define _DEFINES_H_H

#include "log.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <vector>
using namespace std;
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <signal.h>
#include <execinfo.h>

#define MAX_CLIENT 256		//最大接受客户端连接数
#define MAXPACKLEN 1536		//最大包长
#define SERV_COUNT 30
#define SQLLEN 2048
#define NO_DEBUG			//不显示调试信息
#define TIMEOUT 60*10		//客户端套接字超出此时间无操作

typedef enum
{
/************************返回正常应答******************************/
	normal = 0x11,					//正常

/************************非正常应答******************************/
	stationcode_error = 0x40,		//站点编号异常
	period_error = 0x51,			//数据周期错误
	time_id_error = 0x52,			//时间序号错误
	lane_id_scope_error = 0x53,		//车道号范围错误
	lane_id_repeat_error = 0x54,	//车道号重复
	content_type_error = 0x55,		//调查内容不匹配
	data_repeat_error = 0x56,		//数据重复错误
	device_info_error = 0x57,		//设备信息错误
	car_volume_error = 0x58,		//车流量为负
	car_speed_error = 0x59,			//车速为负
	lane_id_numb_error = 0x5A,		//包内车道数与实际车道数不符
	record_time_early_error = 0x61,	//时间超前1小时
	record_time_late_error = 0x62,	//时间超过截止时间
	volume_logic_error = 0x71,		//车流量逻辑错误
	speed_logic_error = 0x72,		//车速逻辑错误
	speed_max_error = 0x73,			//车速最大值错误
	speed_avg_error = 0x74,			//车速平均值错误
	volume_speed_ratio_error = 0x75,//量速比错误
	followpercent_error = 0x81,		//跟车百分比错误，不在0-100之间则保留为0
	time_percentage_error = 0x82,	//时间占比错误,不在0-100之间则保留为0
	avg_distance_error = 0x83,		//平均车头间距错误
	lanecode_order = 0x84,			//车道排序错误
	hardware_error = 0x90,			//硬件错误
	crc_check_error = 0xA0,			//CRC校验错误
	axle_number_error = 0xF1,		//轴数错误
	level_count_error = 0xF3,		//分级数错误(轴载数据的轴数分级，轴重分级)
	stationcode_not_found = 0xF5,	//站点未找到

/************************内部使用******************************/
	protocol_error = 0x10,			//协议错误,数据包大小、数据包尾值、数据包类型、年月日、无对应设备、设备停测
	oracle_fail = 0xF0,				//操作数据库失败
	log_fail = 0xF4					//日志写入失败，数据插入成功(数据插入失败时，不返回日志写入失败)
}ResponeType;

typedef struct{
	int num;
	char name[20];
}sigInfo;

/* 数据库信息 */
struct DbInfo{
	std::ostringstream sqls_insert_traffic_01;	//交调一类
	std::ostringstream sqls_insert_traffic_02;	//交调二类
	std::ostringstream sqls_insert_traffic_03;	//交调三类
	std::ostringstream sqls_insert_weight;		//车总重
	std::ostringstream sqls_insert_realtime;	//单车
	std::ostringstream sqls_insert_axlweight;	//轴重
	std::ostringstream sqls_insert_axlnumber;	//轴数
	std::ostringstream sqls_insert_exceed;		//劝返
	std::ostringstream sqls_insert_recvlog;		//采集日志
	std::ostringstream sqls_insert_errorlog;	//错误日志
	char strDataConnect[256];					//数据库连接字符串
	char strLogsConnect[256];					//日志库连接字符串
};

/* 客户端信息 */
typedef struct
{
	int sockfd;
	int portno;
	char ipaddr[16];
	time_t lasttime;
}ClntInfo;

typedef struct
{
	int sockfd;
	int portno;
}ServInfo;

/*
	** 分类交通量数据
	** 支持保留字段，由外部程序来决定调查内容、是否读写保留字段
*/
struct vehicle_data
{
    int16_t v_numbers;//流量,       2字节
    unsigned char avg_speed;//速度, 1字节
    int16_t reserved_1;//预留位1,   2字节
    int16_t reserved_2;//预留位2,   2字节
};

/*
 * 车道数据
 * 支持I、II、III类交调设备，由外部程序来决定是那类设备
 * */
struct lane_data
{
	unsigned char lane_id;			//车道号，1字节
	unsigned char percentage;		//跟车百分比，1字节
	int16_t avg_distance;			//平均车头距离,2字节
	unsigned char time_percentage;		//时间占比，1字节
	std::vector<vehicle_data> vehicles;	//分类交通量数据，I类9种、II类6种、III类1种。
};

/*车重、轴重、轴数数据
 *轴数数据不包含当量轴次
 */
struct axlgrade_data
{
	unsigned char grade;			//重量段，重量段，轴数
	int32_t volume;					//车总数2字节，轴总数4字节，车总数2字节
	int32_t weight;					//车总重，轴总重，车总重
	int32_t equivalentnumber;		//当量轴次，当量轴次，无
};

/*车重、轴重、轴数数据
 *轴数数据的levelcount是轴数分段数
 *车重、轴重数据是重量分段数
 * */
struct lane_axle
{
	unsigned char lane_id;					//车道号
	int16_t levelcount;						//分段数
	std::vector<axlgrade_data> axldata;		//分段车重、轴重、轴数数据
};

/*交调数据*/
typedef struct
{
	unsigned char i_header[3];			//内层包头，2字节
	int16_t i_packet_length;			//包长，2字节
	unsigned char i_command_type;		//命令字，1字节
	char devcode[17];					//设备编号，16字节
	char stationcode[16];				//站点编号，15字节
	unsigned char deverr;				//硬件错误
	unsigned char content_type;			//调查内容，1字节,1代表单一用途交通流量调查设备；2代表多用途调查设备兼顾交通流量调查功
	unsigned char content_type2;		/*调查内容,1/2，1字节,设备级别,1代表I级设备；2代表II级设备；3代表具备交通密度数据采集功能的III级设备；
						 					 4代表不具备交通密度数据采集功能的III级设备,I级9种车型，II级6种车型，III级2种车型*/
	int16_t year;						//年,2字节
	unsigned char month;				//月，1字节
	unsigned char day;					//日，1字节
	unsigned char period;				//周期，1字节
	int16_t time_id;					//时间序号,2字节
	unsigned char lane_num;				//车道数,1字节
	std::vector<lane_data> lanes;		//车道数据
	unsigned short i_crc16;				//内层CRC校验
	unsigned char i_tailer[3];			//内层包尾
	
	int device_type;					//设备类型、I类、II类、III类，来自设备编号
	char record_time[32];				//检测时间，数据采集时
	char receive_time[32];				//接收时间，数据接收时
	
	int16_t s_id;						//站点ID,2字节
	int16_t t_id;						//设备ID,2字节
}stuInTraffic;

/*单车数据*/
typedef struct
{
	unsigned char i_header[3];			//内层包头
	int16_t i_packet_length;			//包长
	unsigned char i_command_type;		//命令字
	char stationcode[16];				//站点编号
	char devcode[17];					//设备编号
	unsigned char deverr;				//硬件错误
	int32_t vehicleno;					//车辆序号
	int16_t year;						//年
	unsigned char month;				//月
	unsigned char day;					//日
	unsigned char lanenumb;				//车道数
	unsigned char lanecode;				//车道号
	unsigned char hour;					//时
	unsigned char minute;				//分
	unsigned char second;				//秒
	unsigned char vehtype;				//车型
	unsigned char axltype;				//轴型，轴数
	int32_t weight;						//车重
	int16_t speed;						//车速
	int16_t axlweight[10];				//轴重
	int16_t distance[9];				//轴间轴距
	int16_t wheelbase;					//总轴距
	unsigned short i_crc16;				//内层CRC校验
	unsigned char i_tailer[3];			//内层包尾
	
	char record_time[32];				//检测时间，数据采集时
	char receive_time[32];				//接收时间，数据接收时
	
	int16_t s_id;						//站点ID,2字节
	int16_t t_id;						//设备ID,2字节
}stuInRealtime;

/*车重数据*/
typedef struct
{
	unsigned char i_header[3];			//内层包头
	int16_t i_packet_length;			//包长
	unsigned char i_command_type;		//命令字
	char devcode[17];					//设备编号
	char stationcode[16];				//站点编号
	unsigned char deverr;				//硬件错误
	unsigned char devtype;				//设备类型
	int16_t year;						//年,2字节
	unsigned char month;				//月，1字节
	unsigned char day;					//日，1字节
	unsigned char period;				//周期，1字节
	int16_t time_id;					//时间序号,2字节
	unsigned char lanecount;			//车道数
	std::vector<lane_axle> lanes;		//车道数据
	unsigned short i_crc16;				//内层CRC校验
	unsigned char i_tailer[3];			//内层包尾
	
	char record_time[32];				//检测时间，数据采集时
	char receive_time[32];				//接收时间，数据接收时
	
	int16_t s_id;						//站点ID,2字节
	int16_t t_id;						//设备ID,2字节
}stuInWeight;

/*轴数数据*/
typedef struct
{
	unsigned char i_header[3];			//内层包头
	int16_t i_packet_length;			//包长
	unsigned char i_command_type;		//命令字
	char devcode[17];					//设备编号
	char stationcode[16];				//站点编号
	unsigned char deverr;				//硬件错误
	int16_t year;						//年,2字节
	unsigned char month;				//月，1字节
	unsigned char day;					//日，1字节
	unsigned char period;				//周期，1字节
	int16_t time_id;					//时间序号,2字节
	unsigned char lanecount;			//车道数
	std::vector<lane_axle> lanes;		//车道数据
	unsigned short i_crc16;				//内层CRC校验
	unsigned char i_tailer[3];			//内层包尾
	
	char record_time[32];				//检测时间，数据采集时
	char receive_time[32];				//接收时间，数据接收时
	
	int16_t s_id;						//站点ID,2字节
	int16_t t_id;						//设备ID,2字节
}stuInAxlNumber;

/*轴重数据*/
typedef struct
{
	unsigned char i_header[2];			//内层包头
	int16_t i_packet_length;			//包长
	unsigned char i_command_type;		//命令字
	char devcode[17];					//设备编号
	char stationcode[16];				//站点编号
	unsigned char deverr;				//硬件错误
	int16_t year;						//年,2字节
	unsigned char month;				//月，1字节
	unsigned char day;					//日，1字节
	unsigned char period;				//周期，1字节
	int16_t time_id;					//时间序号,2字节
	unsigned char lanecount;			//车道数
	std::vector<lane_axle> lanes;		//车道数据
	unsigned short i_crc16;				//内层CRC校验
	unsigned char i_tailer[2];			//内层包尾
	
	char record_time[32];				//检测时间，数据采集时
	char receive_time[32];				//接收时间，数据接收时
	
	int16_t s_id;						//站点ID,2字节
	int16_t t_id;						//设备ID,2字节
}stuInAxlWeight;

/*劝返数据*/
typedef struct
{
	time_t createtime;				//发送时间，8字节
	char stationcode[16];			//站点编号，ASCII，来自--15字节。
	char devcode[17];				//设备编号，ASCII，来自--16字节。
	unsigned char deverr;			//硬件错误码，1字节
	int32_t vehicle;				//车辆序号，4字节，低位在前，高位在后
	unsigned char lane_num;			//车道数，1字节
	unsigned char lane_code;		//车道号，1字节
	unsigned char vehicle_type;		//车型，1字节，0-9
	unsigned char axletree_type;	//轴型，1字节，两轴车02，三轴车03，七轴及以上07表示
	int32_t vehicle_length;			//车长，4字节，单位mm
	int32_t vehicle_width;			//车宽，4字节，单位mm
	int32_t vehicle_high;			//车高，4字节，单位mm
	unsigned char platetno[16];		//车牌号，12字节，空余部分补0x20
	unsigned char platet_color[4];	//车牌颜色，2字节，ASCII码'黄'，未识别出来则为0x2020
	char exceed_type[4];			//超限类型，4字节，见附表1
	int32_t weight;					//车总重，4字节，单位KG
	int32_t exceed_sign;			//超限标识，4字节，1超限，0不超限
	int32_t equivalentaxle;			//当量轴次
	time_t checktime;				//日期时间，8字节，time_t类型
	int32_t speed;					//速度，4字节
	int32_t acceleration;			//加速度,4字节
	int32_t leftwheelwt1;			//左轮1重，4字节
	int32_t leftwheelwt2;			//左轮2重，4字节
	int32_t leftwheelwt3;			//左轮3重，4字节
	int32_t leftwheelwt4;			//左轮4重，4字节
	int32_t leftwheelwt5;			//左轮5重，4字节
	int32_t leftwheelwt6;			//左轮6重，4字节
	int32_t leftwheelwt7;			//左轮7重，4字节
	int32_t leftwheelwt8;			//左轮8重，4字节
	int32_t rightwheelwt1;			//右轮1重，4字节
	int32_t rightwheelwt2;			//右轮2重，4字节
	int32_t rightwheelwt3;			//右轮3重，4字节
	int32_t rightwheelwt4;			//右轮4重，4字节
	int32_t rightwheelwt5;			//右轮5重，4字节
	int32_t rightwheelwt6;			//右轮6重，4字节
	int32_t rightwheelwt7;			//右轮7重，4字节
	int32_t rightwheelwt8;			//右轮8重，4字节
	int32_t wheelbase1;				//轴距1，4字节
	int32_t wheelbase2;				//轴距2，4字节
	int32_t wheelbase3;				//轴距3，4字节
	int32_t wheelbase4;				//轴距4，4字节
	int32_t wheelbase5;				//轴距5，4字节
	int32_t wheelbase6;				//轴距6，4字节
	int32_t wheelbase7;				//轴距7，4字节
	char imagesource[128];			//图片原始数据（包含路径，图片名）
	int32_t imagelen;				//图片长度

	char imagepath[32];				//图片路径
	char imagename[32];				//图片名

	char recordtime[32];			//记录时间
	char servertime[32];			//接收时间
	char imgname1[64];				//大图名
	char imgpath1[128];				//大图路径
	char imgname2[64];				//车牌识别图名
	char imgpath2[128];				//车牌图路径
	char imgname3[64];				//全景切割图名
	
	int16_t s_id;						//站点ID,2字节
	int16_t t_id;						//设备ID,2字节
	char imgpath3[128];				//全景切割图路径
}stuInExceed;

/*外层包数据*/
typedef struct
{
	char header[2];						// 外包头
	unsigned short packet_length;		// 包长,来自对应的包长度字段2字节,总长度-8
	unsigned char command_type;			// 命令字，来自--1字节
	int16_t dsc_id ;					// DSC客户端编号，来自--2字节
	int32_t packet_id_p1 ;				// 数据包id，来自数据唯一标识，6字节，前4个字节UNIXtime，
	int16_t packet_id_p2 ;				// 后2个字节自增号；或者完全自增号
	unsigned char is_retried;			// 是否重发数据，来自--1字节，"0"非重复,"1"重复
	unsigned char error_code;			// 故障码，来自--1字节
	int32_t retried;					// 重传次数，来自--4字节
	bool is_crc_ensured;				// 外层包CRC校验结果
	unsigned short crc16;				// CRC校验码，来自--2字节，高位在前,低位在后
	char tailer[2];						// 外包尾
}stuOutMessage;

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

	int16_t t_id;
	int16_t s_id;
}stuLogInfo;

/*站点信息*/
struct StationInfo
{
	char stationcode[20];			//站点编号
	char devicecode[20];			//设备编号
	int deviceid;					//设备表ID
	int stationid;					//站点表ID
};

#endif 
