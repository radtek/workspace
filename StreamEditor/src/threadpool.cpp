// =====================================================================================
//	Copyright (C) 2019 by Jiaxing Shao.All rights reserved.
//	文 件 名:  threadpool.cpp
//	作    者:  Jiaxing Shao, 13315567369@163.com
//	版 本 号:  1.0
//	创建时间:  2019年04月17日 18时13分04秒
//	Compiler:  g++
//	描    述:  
// =====================================================================================

#include "threadpool.h"
#include <set>
using namespace std;
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

pthread_mutex_t threadpool_lock;
pthread_cond_t threadpool_cond;
t_threadpool_task **task_list = NULL;
pthread_t *pid = NULL;
int thread_num = 0;
int task_total;
int task_size;
int task_head;
int task_tail;
bool threadpool_shutdown;
// 任务ID
pthread_mutex_t set_lock;
set<unsigned int> setTaskIds;

t_threadpool_task *create_threadpool_task()
{
	t_threadpool_task *task = (t_threadpool_task*)malloc(sizeof(t_threadpool_task));
	task->callback = NULL;
	task->arg = NULL;
	task->release = false;
	task->taskId = 0;
}

void free_threadpool_task(t_threadpool_task *&task)
{
	if(task != NULL)
	{
		if(task->release)
		{
			free(task->arg);
			task->arg = NULL;
		}
		free(task);
		task = NULL;
	}
}

void remove_task_id(int taskId)
{
	if(taskId != 0)
	{
		pthread_mutex_lock(&set_lock);
		set<unsigned int>::iterator iter = setTaskIds.find(taskId);
		if(iter != setTaskIds.end())
		{
			setTaskIds.erase(iter);
		}
		pthread_mutex_unlock(&set_lock);
	}
}

void threadpool_start(int num, int max_task_num)
{
	// 线程池参数信息初始化
	pthread_mutex_init(&threadpool_lock, NULL);
	pthread_cond_init(&threadpool_cond, NULL);
	pthread_mutex_init(&set_lock, NULL);
	task_list = (t_threadpool_task**)malloc(max_task_num * sizeof(t_threadpool_task*));
	memset(task_list, 0, max_task_num * sizeof(t_threadpool_task*));
	pid = (pthread_t*)malloc(num * sizeof(pthread_t));
	thread_num = num;
	task_total = max_task_num;
	task_size = 0;
	task_head = 0;
	task_tail = 0;
	threadpool_shutdown = false;

	for(int i = 0; i < thread_num; i++)
	{
		pthread_create(&pid[i], NULL, threadpool_func, NULL);
	}
}

void threadpool_stop()
{
	threadpool_shutdown = true;
	pthread_cond_broadcast(&threadpool_cond);
	for(int i = 0; i < thread_num; i++)
	{
		pthread_join(pid[i], NULL);
	}
	if(pid != NULL)
	{
		free(pid);
		pid = NULL;
	}
	if(task_list != NULL)
	{
		int n = 0;
		for(int i = 0; i < task_size; i++)
		{
			if(task_list[task_head] != NULL)
			{
				if(task_list[task_head]->arg != NULL && task_list[task_head]->release)
				{
					free(task_list[task_head]->arg);
					task_list[task_head]->arg = NULL;
				}
				free(task_list[task_head]);
				task_list[task_head] = NULL;
				task_head = (task_head + 1) % task_total;
			}
		}
		free(task_list);
		task_list = NULL;
	}
	
	pthread_cond_destroy(&threadpool_cond);
	pthread_mutex_destroy(&threadpool_lock);
}

int threadpool_add_task(t_threadpool_task* &task)
{
	if(task == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&threadpool_lock);
	if(task_total > task_size)
	{
		task_list[task_tail] = task;
		task_tail = (task_tail + 1) % task_total;
		task_size += 1;
		pthread_mutex_unlock(&threadpool_lock);
		pthread_cond_signal(&threadpool_cond);
		return 0;
	}
	else
	{
		pthread_mutex_unlock(&threadpool_lock);
		return -1;
	}
}

int threadpool_add_task(t_threadpool_task* &task, int taskId)
{
	if(task == NULL)
	{
		return -1;
	}

	if(taskId != 0)
	{
		pthread_mutex_lock(&set_lock);
		if(setTaskIds.find(taskId) != setTaskIds.end())
		{
			pthread_mutex_unlock(&set_lock);
			free_threadpool_task(task);
			return 1;
		}
		pthread_mutex_unlock(&set_lock);
	}

	task->taskId = taskId;
	pthread_mutex_lock(&set_lock);
	setTaskIds.insert(taskId);
	pthread_mutex_unlock(&set_lock);

	pthread_mutex_lock(&threadpool_lock);
	if(task_total > task_size)
	{
		task_list[task_tail] = task;
		task_tail = (task_tail + 1) % task_total;
		task_size += 1;
		pthread_mutex_unlock(&threadpool_lock);
		pthread_cond_signal(&threadpool_cond);
		return 0;
	}
	else
	{
		pthread_mutex_unlock(&threadpool_lock);
		return -1;
	}
}

void *threadpool_func(void *arg)
{
	while(true)
	{
		pthread_mutex_lock(&threadpool_lock);
		while(task_size == 0 && !threadpool_shutdown)
		{
			pthread_cond_wait(&threadpool_cond, &threadpool_lock);
			// 线程池结束
		}
		
		if(threadpool_shutdown)
		{
			pthread_mutex_unlock(&threadpool_lock);
			pthread_exit(NULL);
		}

		t_threadpool_task *task = task_list[task_head];
		task_head = (task_head + 1) % task_total;
		task_size -= 1;
		pthread_mutex_unlock(&threadpool_lock);

		task->callback(task->arg);
		remove_task_id(task->taskId);
		free_threadpool_task(task);
	}
	return NULL;
}

