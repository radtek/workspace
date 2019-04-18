// =====================================================================================
//	Copyright (C) 2019 by Jiaxing Shao.All rights reserved.
//	文 件 名:  rtsp_server.h
//	作    者:  Jiaxing Shao, 13315567369@163.com
//	版 本 号:  1.0
//	创建时间:  2019年04月10日 10时13分32秒
//	Compiler:  g++
//	描    述:  
// =====================================================================================

#ifndef RTSP_SERVER_H_H_H
#define RTSP_SERVER_H_H_H

#include "rtsp_task.h"
#include "rtsp_struct.h"
#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>

// 开启端口
int create_server_socket(int port);
// 返回对象
tcp_server_info *create_tcp_server(char *localip, char *port);
// 释放内存
void free_tcp_server(tcp_server_info* &info);
// rtsp应答
void rtsp_response(void *arg);
// 接收rtsp
int recv_rtsp_command(t_device_video_play *player, int sockfd);

#endif

