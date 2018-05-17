#include "pthreadpool.h"
using namespace std;

/* 静态成员初始化 */
vector<CTask*> ThreadPool::m_vecTaskList;
bool ThreadPool::shutdown = false;
pthread_mutex_t ThreadPool::m_pthreadMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t ThreadPool::m_pthreadCond = PTHREAD_COND_INITIALIZER;

bool ThreadPool::m_bIsConn = false;
pthread_mutex_t ThreadPool::m_transMutex = PTHREAD_MUTEX_INITIALIZER;
int ThreadPool::m_iTransSock = 0;

ThreadPool::ThreadPool(int threadnum)
{
	this->m_iThreadNum = threadnum;
	cout<<GetSystemTime()<<": Create "<< threadnum <<" thread"<<endl;
	pthread_id = (pthread_t*)malloc(sizeof(pthread_t) * threadnum);
	for(int i=0;i<m_iThreadNum;i++)
	{
		pthread_create(&pthread_id[i],NULL,ThreadFunc,NULL);
	}
}

int ThreadPool::StopThreadPool()
{
	if(shutdown)
	{
		return -1;
	}

	shutdown = true;
	cout<<"End all thread!"<<endl;
	/* 唤醒所有阻塞线程 */
	pthread_cond_broadcast(&m_pthreadCond);
	for(int i=0;i<m_iThreadNum;i++)
	{
		pthread_join(pthread_id[i],NULL);
	}

	if(pthread_id != NULL)
	{
		free(pthread_id);
		pthread_id = NULL;
	}
	
	pthread_mutex_destroy(&m_pthreadMutex);
	pthread_cond_destroy(&m_pthreadCond);
	pthread_mutex_destroy(&m_transMutex);
	return 0;
}

int ThreadPool::AddTask(CTask* task)
{
	pthread_mutex_lock(&m_pthreadMutex);
	this->m_vecTaskList.push_back(task);
	pthread_mutex_unlock(&m_pthreadMutex);
	pthread_cond_signal(&m_pthreadCond);
	return 0;
}

void* ThreadPool::ThreadFunc(void * arg)
{
	pthread_t tid = pthread_self();
	while(1)
	{
		pthread_mutex_lock(&m_pthreadMutex);
		while(m_vecTaskList.size() == 0 && !shutdown)
		{
			pthread_cond_wait(&m_pthreadCond,&m_pthreadMutex);
		}

		if(shutdown)
		{
			pthread_mutex_unlock(&m_pthreadMutex);
			cout<<"Thread id:"<<tid<<" will exit!"<<endl;
			pthread_exit(NULL);
		}

		vector<CTask*>::iterator iter = m_vecTaskList.begin();
		
		CTask* task = *iter;
		if(iter != m_vecTaskList.end())
		{
			m_vecTaskList.erase(iter);
		}
		pthread_mutex_unlock(&m_pthreadMutex);
	
		/* 执行任务 */
		task->parseresult = task->Run();		
		if(task->parseresult == 0x11 && (task->type == 1 || task->type == 57 || task->type == 97 || task->type == 98 || task->type == 104))
		{	/* 转发交调以及轴载数据 */
			pthread_mutex_lock(&m_transMutex);
			if(m_bIsConn){
				int result = task->Send(m_iTransSock);
				if(result > 0)
				{
					if(task->type == 1)
						g_logs.WriteLog("交调数据转发成功");
					else if(task->type == 57)	
						g_logs.WriteLog("单车数据转发成功");
					else if(task->type == 97)	
						g_logs.WriteLog("车重数据转发成功");
					else if(task->type == 98)	
						g_logs.WriteLog("轴数数据转发成功");
					else if(task->type == 104)	
						g_logs.WriteLog("轴重数据转发成功");
				}
			}
			pthread_mutex_unlock(&m_transMutex);
		}
		/* 释放 */
		if(task != NULL)
		{
			delete task;
			task = NULL;
		}
	}
	return NULL;
}

