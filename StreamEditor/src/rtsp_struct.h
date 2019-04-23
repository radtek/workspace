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

#include <sys/select.h>
#include "bytearray.h"
#include "rtsp_protocol.h"

#define MAX_BUF_SIZE (2048)
#define MAX_CLNT_ONLINE (32)
#define MAX_VIDEO_CACHE (1024 * 16)
#define MAX_SERVICE_WAIT_TIME (300)
#define MAX_CLIENT_COUNT (32)
#define MAX_DEVICE_COUNT (512)

typedef struct 
{
	// IP地址
	char ipaddr[16];
	// 端口号
	int port;
	// 套接字
	int sockfd;
}tcp_client_info;

typedef struct
{
	int deviceid;
	// 是否需要释放
	bool ready_stop;
	// 是否在使用
	bool stop;
	// 无连接时间,单位:秒, 60秒无连接则关闭服务端口
	int time_count;
	// 客户端连接数
	int clnt_count;
	// client基本信息
	int clntfd[MAX_CLIENT_COUNT];
}t_device_clnt;

typedef struct
{
	char vir_url[128];
	// 本地IP
	char ipaddr[16];
	// 开辟的端口号
	int port;
	// 服务端套接字
	int sockfd;
	// 
	int maxfd;
	//
	fd_set fds;
	// 锁
	pthread_mutex_t lock;
	// device count
	int device_count;
	// 最大支持128个设备
	t_device_clnt *device[MAX_DEVICE_COUNT];
}tcp_server_info;

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
} t_device_info;

typedef struct
{
	// 到设备的连接套接字
	int sockfd;
	// 对应得server服务下标
	int serv_pos;
	// 设备基础信息
	t_device_info *device_info;
	// rtsp基本信息
	t_rtsp_info *dev_rtsp_info;
	// 虚拟rtsp信息
	t_rtsp_info *vir_rtsp_info;
	// 存储数据
	t_byte_array *rtp_array;
	// 锁
	pthread_mutex_t lock;
	// 线程ID
	pthread_t pid[2];
}t_device_video_play;

#endif
