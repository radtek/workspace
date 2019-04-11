// =====================================================================================
//	Copyright (C) 2019 by Jiaxing Shao.All rights reserved.
//	文 件 名:  logfile.cpp
//	作    者:  Jiaxing Shao, 13315567369@163.com
//	版 本 号:  1.0
//	创建时间:  2019年02月15日 15时43分08秒
//	Compiler:  g++
//	描    述:  
// =====================================================================================

#include "logfile.h"

static LOG_QUEUE *log_queues[MAX_LOG_QUEUE_COUNT];
static int log_count = 0;
static pthread_mutex_t log_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
static char log_root_directory[32] = "./logs";

int record_log(LOG_QUEUE *log_queue, const char *format, ...)
{
	LOG *log = GET_PRODUCER(log_queue);
	if(log == NULL)
	{
		// cout << "error" << endl;
		return -1;
	}
	log->fd = log_queue->fd;
	va_list args;
	va_start(args, format);
	vsprintf(log->buf, format, args);
	va_end(args);
	PUT_PRODUCER(log_queue);
	return 0;
}

LOG_QUEUE *create_log_queue(const char *log_name)
{
	pthread_mutex_lock(&log_queue_mutex);
	if(log_count >= MAX_LOG_QUEUE_COUNT)
	{
		// cout << "log queue count is overflow!" << endl;
		pthread_mutex_unlock(&log_queue_mutex);
		return NULL;
	}
	LOG_QUEUE *log_queue = (LOG_QUEUE*)malloc(sizeof(LOG_QUEUE) + sizeof(LOG) * MAX_LOG_QUEUE_SIZE);
	if(log_queue == NULL)
	{
		// cout << "create log queue error!" << endl;
		pthread_mutex_unlock(&log_queue_mutex);
		return NULL;
	}
	memset(log_queue, 0, sizeof(LOG_QUEUE) + sizeof(LOG) * MAX_LOG_QUEUE_SIZE);
	log_queue->size = MAX_LOG_QUEUE_SIZE;
	// 检查目录是否存在,没有则新建
	if(opendir(log_root_directory) == NULL)
	{
		mkdir(log_root_directory, S_IRWXU);
	}
	char log_file_path[MAX_LOG_FILE_PATH] = { 0 };
	sprintf(log_file_path, "%s//%s.txt", log_root_directory, log_name);
	log_queue->fd = open(log_file_path, O_RDWR | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
	log_queues[log_count++] = log_queue;
	pthread_mutex_unlock(&log_queue_mutex);
	return log_queue;
}

static void *log_worker_func(void *arg)
{
	while(true)
	{
		int count = 0;
		for(int i = 0; i < log_count; i++)
		{
			LOG_QUEUE *queue = log_queues[i];
			LOG *log = GET_CONSUMER(queue);
			if(log == NULL)
			{
				count++;
				continue;
			}
			write(log->fd, log->buf, strlen(log->buf));
			PUT_CONSUMER(queue);
		}

		if(log_count == count)
		{
			sleep(1);
		}
	}
	return NULL;
}

void start_log_thread()
{
	pthread_t tid;
	pthread_create(&tid, NULL, log_worker_func, NULL);
}

