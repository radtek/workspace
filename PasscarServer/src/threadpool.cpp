#include "threadpool.h"

//静态成员初始化
vector<CTask*> ThreadPool::m_vecTaskList;
bool ThreadPool::shutdown = false;
pthread_mutex_t ThreadPool::m_pthreadMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t ThreadPool::m_pthreadCond = PTHREAD_COND_INITIALIZER;

ThreadPool::ThreadPool(int threadnum)
{
	pthread_id = (pthread_t*)malloc(sizeof(pthread_t) * threadnum);
	m_iThreadNum = threadnum;
	for(int i = 0; i < threadnum; i++)
	{
		pthread_create(&pthread_id[i], NULL, ThreadFunc, NULL);
	}
}

ThreadPool::~ThreadPool()
{
	Destroy();
	pthread_mutex_destroy(&m_pthreadMutex);
	pthread_cond_destroy(&m_pthreadCond);
}

void ThreadPool::Destroy()
{
	if(shutdown)
	{
		return;
	}

	shutdown = true;

	//唤醒所有阻塞进程
	pthread_cond_broadcast(&m_pthreadCond);
	for(int i = 0; i < m_iThreadNum; i++)
	{
		//等待线程结束
		pthread_join(pthread_id[i], NULL);
	}

	if(pthread_id != NULL)
	{
		free(pthread_id);
		pthread_id = NULL;
	}
}

void ThreadPool::AddTask(CTask* task)
{
	pthread_mutex_lock(&m_pthreadMutex);
	m_vecTaskList.push_back(task);
	pthread_mutex_unlock(&m_pthreadMutex);
	pthread_cond_signal(&m_pthreadCond);
}

void* ThreadPool::ThreadFunc(void* arg)
{
	pthread_t tid = pthread_self();
	while(true)
	{
		pthread_mutex_lock(&m_pthreadMutex);
		while(m_vecTaskList.size() == 0 && !shutdown)
		{
			pthread_cond_wait(&m_pthreadCond,&m_pthreadMutex);
		}

		if(shutdown)
		{
			pthread_mutex_unlock(&m_pthreadMutex);
			pthread_exit(NULL);
		}

		//取第一条任务
		vector<CTask*>::iterator iter = m_vecTaskList.begin();
		CTask* task = *iter;
		if(iter != m_vecTaskList.end())
		{
			m_vecTaskList.erase(iter);
		}
		pthread_mutex_unlock(&m_pthreadMutex);

		int result = task->Run();	//执行任务
		if(result != 0xA0 && result != 0xA1 && result != 0xA2)
		{
			result = send(task->m_sock, task->response, RESPONSE_SIZE, 0);
#ifndef NO_DEBUF
			if(result < 0)
			{
				g_logs->WriteLog("Return response error.");
			}
#endif			
		}

		if(task != NULL)
		{
			delete task;
			task = NULL;
		}
	}
}

