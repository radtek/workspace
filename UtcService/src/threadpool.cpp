// =====================================================================================
//	Copyright (C) 2018 by Jiaxing Shao.All rights reserved.
//	文 件 名:  threadpool.cpp
//	作    者:  Jiaxing Shao, 13315567369@163.com
//	版 本 号:  1.0
//	创建时间:  2018年06月28日 11时22分37秒
//	Compiler:  g++
//	描    述:  
// =====================================================================================

#include "threadpool.h"

vector<T_Task*> ThreadPool::m_vecTask;
pthread_mutex_t ThreadPool::m_mutexThreadPool = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t ThreadPool::m_condThreadPool = PTHREAD_COND_INITIALIZER;

ThreadPool::ThreadPool()
{
	char thread_num[10] = { 0 };
	GetConfigureString("ThreadPoolNum", thread_num, 10, "10", CONFFILE);
	m_iThreadNum = atoi(thread_num);
	pthread_t *tid = new pthread_t[m_iThreadNum];
	for(int i = 0; i < m_iThreadNum; i++)
	{
		pthread_create(&tid[i], NULL, ThreadFunc, this);
		pthread_detach(tid[i]);
	}
}

void *ThreadPool::ThreadFunc(void *arg)
{
	ThreadPool *pthis = (ThreadPool*)arg;
	pthread_t tid = pthread_self();
	while(true)
	{
		pthread_mutex_lock(&m_mutexThreadPool);
		while(pthis->GetTasklistSize() == 0)
		{
			pthread_cond_wait(&m_condThreadPool, &m_mutexThreadPool);
		}
		vector<T_Task*>::iterator iter = m_vecTask.begin();
		T_Task *task = NULL;
		if(iter != m_vecTask.end())
		{
			task = *iter;
			m_vecTask.erase(iter);
		}
		pthread_mutex_unlock(&m_mutexThreadPool);
		
		if(task->type == emDecode)
		{
			pthis->m_xmlCodec.DecodeXmlMessage(task->strXml);
		}
		else
		{
			pthis->m_xmlCodec.EncodeXmlMessage(task->msg);
		}

		if(task != NULL)
		{
			if(task->msg != NULL)
			{
				delete task->msg;
				task->msg = NULL;
			}
			delete task;
			task = NULL;
		}
	}
}

void ThreadPool::AddTask(T_Task *task)
{
	pthread_mutex_lock(&m_mutexThreadPool);
	m_vecTask.push_back(task);
	pthread_mutex_unlock(&m_mutexThreadPool);
	pthread_cond_signal(&m_condThreadPool);
}

