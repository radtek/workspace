// =====================================================================================
//	Copyright (C) 2018 by Jiaxing Shao.All rights reserved.
//	文 件 名:  bytearray.cpp
//	作    者:  Jiaxing Shao, 13315567369@163.com
//	版 本 号:  1.0
//	创建时间:  2018年06月08日 14时24分25秒
//	Compiler:  g++
//	描    述:  
// =====================================================================================

#include "bytearray.h"
#include <iostream>
using namespace std;
#include <string.h>
#include <stdlib.h>

t_byte_array *create_byte_array(int size)
{
	t_byte_array *byte_array = (t_byte_array*)malloc(sizeof(t_byte_array));
	byte_array->buffer = (char*)malloc(size);
	memset(byte_array->buffer, 0, size);
	byte_array->total = size;
	byte_array->size = 0;
	byte_array->head = 0;
	byte_array->tail = 0;
	byte_array->stop = true;
	pthread_mutex_init(&byte_array->lock, NULL);
	pthread_cond_init(&byte_array->cond, NULL);
	return byte_array;
}

void free_byte_array(t_byte_array* &byte_array)
{
	if(byte_array == NULL)
	{
		return;
	}

	byte_array->stop = true;
	pthread_cond_signal(&byte_array->cond);
	if(byte_array != NULL)
	{
		if(byte_array->buffer != NULL)
		{
			free(byte_array->buffer);
			byte_array->buffer = NULL;
		}
		pthread_mutex_destroy(&byte_array->lock);
		pthread_cond_destroy(&byte_array->cond);
		free(byte_array);
		byte_array = NULL;
	}
}

int put_byte_array(t_byte_array *byte_array, const char *buf, int len)
{
	if(byte_array == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&byte_array->lock);
	if(byte_array->total - byte_array->size >= len)
	{
		if(byte_array->total - byte_array->tail >= len)
		{
			memcpy(byte_array->buffer + byte_array->tail, buf, len);
			byte_array->tail += len;
		}
		else
		{
			int llen = byte_array->total - byte_array->tail;
			int rlen = len - llen;
			memcpy(byte_array->buffer + byte_array->tail, buf, llen);
			memcpy(byte_array->buffer, buf + llen, rlen);
			byte_array->tail = rlen;
		}
		byte_array->size += len;
		pthread_mutex_unlock(&byte_array->lock);
		pthread_cond_signal(&byte_array->cond);
		return 0;
	}
	pthread_mutex_unlock(&byte_array->lock);
	return -1;
}

int get_byte_array(t_byte_array *byte_array, char *buf, int len)
{
	if(byte_array == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&byte_array->lock);
	while(byte_array->size < len && !byte_array->stop)
	{
		pthread_cond_wait(&byte_array->cond, &byte_array->lock);
	}

	if(byte_array->stop)
	{
		pthread_mutex_unlock(&byte_array->lock);
		return 0;
	}

	if(byte_array->size >= len)
	{
		if(byte_array->total - byte_array->head >= len)
		{
			memcpy(buf, byte_array->buffer + byte_array->head, len);
			byte_array->head += len;
		}
		else
		{
			int llen = byte_array->total - byte_array->head;
			int rlen = len - llen;
			memcpy(buf, byte_array->buffer + byte_array->head, llen);
			memcpy(buf + llen, byte_array->buffer, rlen);
			byte_array->head = rlen;
		}
		byte_array->size -= len;
		pthread_mutex_unlock(&byte_array->lock);
		return len;
	}
	pthread_mutex_unlock(&byte_array->lock);
	return -1;
}

void clean_byte_array(t_byte_array *byte_array)
{
	pthread_mutex_lock(&byte_array->lock);
	byte_array->stop = true;
	byte_array->size = 0;
	byte_array->head = 0;
	byte_array->tail = 0;
	pthread_mutex_unlock(&byte_array->lock);
	pthread_cond_signal(&byte_array->cond);
}
