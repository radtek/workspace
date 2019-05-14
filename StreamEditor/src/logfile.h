// =====================================================================================
//	Copyright (C) 2019 by Jiaxing Shao.All rights reserved.
//	文 件 名:  logfile.h
//	作    者:  Jiaxing Shao, 13315567369@163.com
//	版 本 号:  1.0
//	创建时间:  2019年02月15日 15时43分21秒
//	Compiler:  g++
//	描    述:  不确保时完全线程安全的日志功能???(测试过程中没有出现问题)
// =====================================================================================

#ifndef __LOGFILE_H_H_H
#define __LOGFILE_H_H_H

#include "functions.h"
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <unistd.h>
#include <dirent.h>

//#define NO_DEBUG

#define MAX_LOG_QUEUE_COUNT (1024 * 10)
#define MAX_LOG_QUEUE_SIZE 	(1024 * 10)
#define MAX_LOG_LINE_LENGTH	(2048)
#define MAX_LOG_FILE_PATH (1024)

#define PUT_CONSUMER(row) (row)->consumer = ((row)->consumer + 1) % (row)->size
#define PUT_PRODUCER(row) (row)->producer = ((row)->producer + 1) % (row)->size
#define GET_CONSUMER(row) ((row)->producer != (row)->consumer) ? &(row)->items[(row)->consumer] : NULL
#define GET_PRODUCER(row) (((row)->producer + 1) % (row)->size != (row)->consumer) ? &(row)->items[(row)->producer] : NULL
#define log_info(queue, format, args...) do{ string str = GetSystemTime(); \
	record_log(queue, "%s [info]: "format"\n", str.c_str(), ##args); }while(0)

#ifndef NO_DEBUG
#define log_debug(format, args...) do{ \
		string log_debug_str = GetSystemTime(); \
		printf("%s [debug]: "format"\n", log_debug_str.c_str(), ##args); \
		}while(0)
#else
#define log_debug(format, args...) do{}while(0)
#endif

typedef struct _LOG
{
	int fd;
	char buf[MAX_LOG_LINE_LENGTH];
} LOG;

typedef struct _LOG_QUEUE
{
	int size;
	int fd;
	int producer;
	int consumer;
	LOG items[0];
} LOG_QUEUE;

LOG_QUEUE *create_log_queue(const char *log_name);
void start_log_thread();
int record_log(LOG_QUEUE *queue, const char *format, ...);

#endif

