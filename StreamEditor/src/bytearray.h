// =====================================================================================
//	Copyright (C) 2018 by Jiaxing Shao.All rights reserved.
//	文 件 名:  bytearray.h
//	作    者:  Jiaxing Shao, 13315567369@163.com
//	版 本 号:  1.0
//	创建时间:  2018年06月08日 14时10分10秒
//	Compiler:  g++
//	描    述:  
// =====================================================================================

#ifndef __BYTEARRAY_H_H_H
#define __BYTEARRAY_H_H_H

#include "rtsp_struct.h"
#include <iostream>
using namespace std;
#include <string.h>
#include <pthread.h>

// 创建对象, 10M
t_rtp_byte_array *rtp_array_create(int size = 1024 * 1024 * 10);
// 主线程
void *byte_array_process_start(void *arg);
// 取出完整rtp数据包
int get_rtp_buffer(t_rtp_byte_array* &rtp_array, unsigned char *buf);
// 放进队列
bool put_byte_array(t_rtp_byte_array* &rtp_array, char *buf, int len);
// 要取出的长度
bool get_byte_array(t_rtp_byte_array* &rtp_array, char *buf, int len);
// 停止服务
void byte_array_process_stop(t_rtp_byte_array* &rtp_array);

#endif

