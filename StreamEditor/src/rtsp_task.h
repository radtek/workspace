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

#include "rtsp_struct.h"

static pthread_mutex_t g_mapVideoLock;

void video_play_free(t_video_play_info* &player);
void video_task_add(int deviceid);
void video_task_remove(int deviceid);
t_video_play_info *video_task_get(int deviceid);

#endif
