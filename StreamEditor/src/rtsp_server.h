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
static void *tcp_server_worker(void *arg);
// 停止线程,并关闭套接字
void tcp_server_stop(t_video_play_info *player);

void on_accept(int fd, short events, void *arg);
void buf_recv_cb(struct bufferevent *bev, void *arg);
void buf_error_cb(struct bufferevent *bev, short events, void *arg);

bool recv_rtsp_command(int sock);

#endif

