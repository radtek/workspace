// =====================================================================================
//	Copyright (C) 2018 by Jiaxing Shao.All rights reserved.
//	文 件 名:  bytearray.cpp
//	作    者:  Jiaxing Shao, 13315567369@163.com
//	版 本 号:  1.0
//	创建时间:  2018年06月08日 14时24分25秒
//	Compiler:  g++
//	描    述:  
// =====================================================================================

#include "rtsp_task.h"
#include "bytearray.h"
#include "logfile.h"
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>

t_rtp_byte_array *rtp_array_create(int size)
{
	t_rtp_byte_array *rtp_array = new t_rtp_byte_array;
	rtp_array->buffer = new char[size];
	memset(rtp_array->buffer, 0, size);
	rtp_array->total = size;
	rtp_array->size = 0;
	rtp_array->head = 0;
	rtp_array->tail = 0;
	rtp_array->stop = false;
	pthread_mutex_init(&rtp_array->lock, NULL);
	pthread_cond_init(&rtp_array->cond, NULL);
	return rtp_array;
}

void *byte_array_process_start(void *arg)
{
	int deviceid = *((int*)arg);
	log_debug("byte_array_process_start 线程启动, deviceid %d", deviceid);
	pthread_detach(pthread_self());
	t_video_play_info *player = video_task_get(deviceid);
	if(player == NULL)
	{
		log_debug("byte_array_process_start 获取 player 失败, deviceid %d, 线程退出", deviceid);
	}
	unsigned char buffer[255 * 255];
	int length = 0;
	while(true)
	{
		if(player->stop)
		{
			log_debug("byte_array_process_start 线程准备退出, deviceid %d", deviceid);
			break;
		}
		
		length = get_rtp_buffer(player->rtp_array, buffer);
		if(buffer[0] != 0x24)
		{
			byte_array_process_stop(player->rtp_array);
			break;
		}

		if(player->rtsp_serv->clnt_count != 0)
		{
			int clnt_count = player->rtsp_serv->clnt_count;
			for(int i = 0; i < MAX_CLNT_ONLINE; i++)
			{
				if(clnt_count == 0)
				{
					break;
				}
				if(player->rtsp_serv->clnt[i] != NULL)
				{
					send(player->rtsp_serv->clnt[i]->sockfd, buffer, length, 0);
					clnt_count -= 1;
				}
			}
		}
	}
	log_debug("byte_array_process_start 线程退出, deviceid %d", deviceid);
	return NULL;
}

void byte_array_process_stop(t_rtp_byte_array* &rtp_array)
{
	rtp_array->stop = true;
}

int get_rtp_buffer(t_rtp_byte_array* &rtp_array, unsigned char *buf)
{
	int length = -1;
	if(get_byte_array(rtp_array, (char*)buf, 4))
	{
		length = buf[2] * 256 + buf[3];
		while(true)
		{
			if(get_byte_array(rtp_array, (char*)buf + 4, length))
			{
				break;
			}
			sleep(0.01);
		}
		length += 4;
	}
	return length;
}

bool put_byte_array(t_rtp_byte_array* &rtp_array, char *buf, int len)
{
	pthread_mutex_lock(&rtp_array->lock);
	if(rtp_array->total - rtp_array->size >= len)
	{
		if(rtp_array->total - rtp_array->tail >= len)
		{
			memcpy(rtp_array->buffer + rtp_array->tail, buf, len);
			rtp_array->tail += len;
		}
		else
		{
			int llen = rtp_array->total - rtp_array->tail;
			int rlen = len - llen;
			memcpy(rtp_array->buffer + rtp_array->tail, buf, llen);
			memcpy(rtp_array->buffer, buf + llen, rlen);
			rtp_array->tail = rlen;
		}
		rtp_array->size += len;
		pthread_mutex_unlock(&rtp_array->lock);
		pthread_cond_signal(&rtp_array->cond);
		return true;
	}
	pthread_mutex_unlock(&rtp_array->lock);
	return false;
}

bool get_byte_array(t_rtp_byte_array* &rtp_array, char *buf, int len)
{
	pthread_mutex_lock(&rtp_array->lock);
	while(rtp_array->size < len)
	{
		if(rtp_array->stop)
		{
			break;
		}
		pthread_cond_wait(&rtp_array->cond, &rtp_array->lock);
	}

	if(rtp_array->size >= len)
	{
		if(rtp_array->total - rtp_array->head >= len)
		{
			memcpy(buf, rtp_array->buffer + rtp_array->head, len);
			rtp_array->head += len;
		}
		else
		{
			int llen = rtp_array->total - rtp_array->head;
			int rlen = len - llen;
			memcpy(buf, rtp_array->buffer + rtp_array->head, llen);
			memcpy(buf + llen, rtp_array->buffer, rlen);
			rtp_array->head = rlen;
		}
		rtp_array->size -= len;
		pthread_mutex_unlock(&rtp_array->lock);
		return true;
	}
	pthread_mutex_unlock(&rtp_array->lock);
	return false;
}

