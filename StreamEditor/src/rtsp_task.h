// =====================================================================================
//	Copyright (C) 2019 by Jiaxing Shao.All rights reserved.
//	文 件 名:  rtsp_task.h
//	作    者:  Jiaxing Shao, 13315567369@163.com
//	版 本 号:  1.0
//	创建时间:  2019年04月11日 17时53分50秒
//	Compiler:  g++
//	描    述:  
// =====================================================================================

#ifndef _RTSP_TASK_H_H_H
#define _RTSP_TASK_H_H_H

#include "otlcontrol.h"
#include "rtsp_struct.h"

// 释放内存
void video_play_free(t_device_video_play* &player);
// 数据库查询设备信息
bool select_device_info(t_db_conn *conn);
// 获取player任务
t_device_video_play *video_task_get(int deviceid);

// 取出完整rtp数据包
int get_rtp_buffer(t_byte_array* &rtp_array, unsigned char *buf);
// 获取设备信息
bool get_device_info();

// 开始到设备rtsp任务
void *rtsp_worker_start(void *arg);
// 服务端工作线程, 处理rtsp请求
void *rtsp_server_start(void *arg);
// 流处理主线程
void *byte_array_process_start(void *arg);
// 线程池任务, 任务结束时，添加针对设备的task任务，来处理订阅的取消
void task_release(void *arg);

/*
void video_task_add(int deviceid);
void video_task_remove(int deviceid);
*/

#endif
