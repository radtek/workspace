// =====================================================================================
//	Copyright (C) 2019 by Jiaxing Shao.All rights reserved.
//	文 件 名:  rtsp_struct.h
//	作    者:  Jiaxing Shao, 13315567369@163.com
//	版 本 号:  1.0
//	创建时间:  2019年04月10日 16时16分03秒
//	Compiler:  g++
//	描    述:  
// =====================================================================================

#ifndef _RTSP_STRUCT_H_H_H
#define _RTSP_STRUCT_H_H_H

#include <string>
using namespace std;

#define MAX_BUF_SIZE (2048)
#define MAX_CLNT_ONLINE (32)

typedef enum 
{
	enum_cmd_options = 0,
	enum_cmd_describe = 1,
	enum_cmd_describe_secret = 2,
	enum_cmd_setup_video = 3,
	enum_cmd_setup_audio = 4,
	enum_cmd_play = 5,
	enum_cmd_teardown = 6
}enum_rtsp_cmd;

typedef struct 
{
	// IP地址
	char ipaddr[16];
	// 端口号
	int port;
	// 套接字
	int sockfd;
	// 需要断开连接时，此位置true
	bool stop;
	// 头标记
	int head;
	// 尾标记
	int tail;
	// 接收缓存
	char buffer[MAX_BUF_SIZE];
}tcp_conn_info;

typedef struct
{
	// 开辟的端口号
	int port;
	// 服务端套接字
	int sockfd;
	// 上次连接时间,5分钟
	time_t last_time;
	// 最大支持32个用户同时访问
	tcp_conn_info *clnt[MAX_CLNT_ONLINE];
}tcp_server_info;

typedef struct
{
	char crossname[64];
	char username[32];
	char password[32];
	char ipaddr[16];
	int port;
	int deviceid;
	int devicetype;
	int rtspport;
}t_device_info;

typedef struct
{
	char rtsp_url[128];
	char video_url[128];
	char audio_url[128];
	char username[32];
	char password[32];
	char session[64];
	char nonce[64];
	char realm[32];
	int cmd_seq;
	bool secret;
}t_rtsp_info;

typedef struct
{
	t_device_info *device_info;
	tcp_conn_info *device_conn;
	tcp_server_info *rtsp_serv;
	t_rtsp_info *rtsp_info;

	string reply_options;
	string reply_describe[2];
	string reply_setup[2];
	string reply_play;
	string reply_teardown;
}t_video_play_info;

typedef struct
{
	unsigned char version;
	unsigned char fillbit;
	unsigned char extend;
	unsigned char csrcCount;
	unsigned char sign;
	unsigned char type;
	unsigned char seq;
	unsigned int timestamp;
	unsigned int ssrc;
	unsigned int *csrc_list;
	unsigned int length;
}t_rtp_head;

#endif
