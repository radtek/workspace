#include "defines.h"
#include "crc16.h"

#ifndef _JDMESSAGE_H_H
#define _JDMESSAGE_H_H

//不去包头，包尾的总长度
static int valid_11 [20] = {105, 137, 169, 201, 233, 265, 297, 329, 361, 393, 425, 457, 489, 521, 553, 585, 617, 649, 681, 713};
static int valid_12 [20] = {141, 209, 277, 345, 413, 481, 549, 617, 685, 753, 821, 889, 957,1025,1093,1161,1229,1297,1365,1433};
static int valid_21 [20] = { 96, 119, 142, 165, 188, 211, 234, 257, 280, 303, 326, 349, 372, 395, 418, 441, 464, 487, 510, 533};
static int valid_22 [20] = {120, 167, 214, 261, 308, 355, 402, 449, 496, 543, 590, 637, 684, 731, 778, 825, 872, 919, 966,1013};
static int valid_31 [20] = { 84,  95, 106, 117, 128, 139, 150, 161, 172, 183, 194, 205, 216, 227, 238, 249, 260, 271, 282, 293};
static int valid_32 [20] = { 92, 111, 130, 149, 168, 187, 206, 225, 244, 263, 282, 301, 320, 339, 358, 377, 396, 415, 434, 453};


extern int iTimeSpace;
extern vector<StationInfo*> vecStation;
extern pthread_mutex_t m_infoMutex;		//获取站点信息

class JdMessage
{
public:
  	JdMessage();
  	~JdMessage();
private:
	int16_t ParseTraffic(unsigned char * packet,int length,stuInTraffic * traffic);		/*解析交调数据，返回故障值，正常返回0*/
	int16_t ParseExceed(unsigned char * packet,int length,stuInExceed * exceed);		/*解析劝返数据，返回故障值，正常返回0*/
	int16_t ParseAxlRealtime(unsigned char * packet,int length,stuInRealtime * realtime);		/*解析单车数据，返回故障值，正常返回0*/
	int16_t ParseAxlNumber(unsigned char * packet,int length,stuInAxlNumber * axlnumber);		/*解析轴数数据，返回故障值，正常返回0*/
	int16_t ParseAxlWeight(unsigned char * packet,int length,stuInAxlWeight * axlweight);		/*解析轴重数据，返回故障值，正常返回0*/
	int16_t ParseCarWeight(unsigned char * packet,int length,stuInWeight * weight);				/*解析车重数据，返回故障值，正常返回0*/
	void CreateTime(char * record_time,int16_t yy,unsigned char mm,unsigned char dd,int16_t time_id);		/*根据时间序号解析时间*/
	bool CompareTime(char * createtime,char * servertime);										/*时间比较,返回比对结果,false表示超过了时间间隔*/
public:
	int16_t ParseMessage(char * source_packet,int16_t length,stuOutMessage &stuMsg,void* &data);	/*解析外层包数据*/
	void AnswerMessage(char * answer_msg,char * source_packet,int16_t code);	/*应答组包*/
	void PrintMessage(void * message,int type);		/*以可读方式打印当前消息*/
};

#endif
