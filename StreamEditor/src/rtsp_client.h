// =====================================================================================
//	Copyright (C) 2019 by Jiaxing Shao.All rights reserved.
//	文 件 名:  rtsp_client.h
//	作    者:  Jiaxing Shao, 13315567369@163.com
//	版 本 号:  1.0
//	创建时间:  2019年04月02日 18时34分55秒
//	Compiler:  g++
//	描    述:  
// =====================================================================================

#include "rtsp_struct.h"

// 创建客户端连接
tcp_conn_info *create_tcp_client_conn(char *ipaddr, int port);
// 开始rtsp任务
void *rtsp_worker_start(void *arg);
// 发送rtsp视频流请求
bool rtsp_request(t_video_play_info *player);
// 封装send rtsp command的系列操作
bool send_rtsp_command(t_video_play_info *player, int type);

bool rtsp_reply_parse(t_rtsp_info *info, char *buffer, int buflen, int type);
