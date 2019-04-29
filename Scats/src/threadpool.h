// =====================================================================================
//	Copyright (C) 2018 by Jiaxing Shao.All rights reserved.
//	文 件 名:  threadpool.h
//	作    者:  Jiaxing Shao, 13315567369@163.com
//	版 本 号:  1.0
//	创建时间:  2018年06月28日 11时22分59秒
//	Compiler:  g++
//	描    述:  
// =====================================================================================

#ifndef __PTHREADPOOL_H_H_H
#define __PTHREADPOOL_H_H_H

#include "log.h"
#include "task.h"
#include <vector>

class ThreadPool
{
public:
	ThreadPool();
	void AddTask(Task *task);
	static void *ThreadFunc(void *arg);
private:
	int GetTasklistSize()
	{
		return m_vecTask.size();
	}
private:
	static vector<Task*> m_vecTask;
	static pthread_mutex_t m_mutexThreadPool;
	static pthread_cond_t m_condThreadPool;
	int m_iThreadNum;
};

#endif
