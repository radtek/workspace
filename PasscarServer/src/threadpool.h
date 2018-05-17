#ifndef THREADPOOL_H_H
#define THREADPOOL_H_H

#include "log.h"
#include "parsemsg.h"
#include <vector>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

class ThreadPool
{
public:
	ThreadPool(int threadnum = 10);	//默认开启10条线程
	~ThreadPool();
	void AddTask(CTask* task);
	void Destroy();
	int getTaskSize()
	{
		return m_vecTaskList.size();
	}

private:
	static std::vector<CTask*> m_vecTaskList;
	static bool shutdown;		//线程池结束标识
	static pthread_mutex_t m_pthreadMutex;		/* 线程同步锁 */
	static pthread_cond_t m_pthreadCond;		/* 线程同步的条件变量 */
	
	int m_iThreadNum;							/* 线程池启动的线程数 */
	pthread_t* pthread_id;
protected:
	static void* ThreadFunc(void* arg);
};

#endif
