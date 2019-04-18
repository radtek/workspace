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

// 连接到服务器
int connect_server(char *ipaddr, int port);
// 发送rtsp视频流请求
bool rtsp_request(t_device_video_play *player);

