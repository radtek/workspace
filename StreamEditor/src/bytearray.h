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

#include <pthread.h>

typedef struct
{
	char *buffer;
	int size;
	int total;
	int head;
	int tail;
	bool stop;
	pthread_mutex_t lock;
	pthread_cond_t cond;
}t_byte_array;

// 创建对象, 10M
t_byte_array *create_byte_array(int size = 1024 * 1024 * 10);
// 放进队列
int put_byte_array(t_byte_array *byte_array, const char *buf, int len);
// 要取出的长度
int get_byte_array(t_byte_array *byte_array, char *buf, int len);
// 
void free_byte_array(t_byte_array* &byte_array);

#endif

