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

#include "rtsp_struct.h"
#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>

// 返回套接字
int create_tcp_server(int port);
// 服务端工作线程, 处理rtsp请求
void *rtsp_server_start(void *arg);
// rtsp应答
bool rtsp_response(t_video_play_info *player, int sockfd);
// 接收rtsp
int recv_rtsp_command(t_video_play_info *player, int sockfd);

int rtsp_cmd_parse(t_rtsp_reply_info *info, char *buffer, int buflen);
#endif

