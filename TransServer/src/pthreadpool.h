#ifndef _PTHREADPOOL_H_H
#define _PTHREADPOOL_H_H

#include <iostream>
#include <vector>
#include <pthread.h>
#include <malloc.h>
#include "defines.h"

/* 执行任务的类 */
class CTask
{
protected:
	char* m_ptrData;
	int m_iLength;
	ClntInfo clnt;
public:
	int type;			//包类型,区分交调、轴载、劝返
	int parseresult;
	CTask(){m_ptrData = NULL;}
	virtual int Run() = 0;						//由派生类定义要执行的任务
	virtual int Send(int sock) = 0;						//数据转发
	void setData(char* data,const ClntInfo &clntinfo,int len)			//需要处理的数据
	{
		m_ptrData = data;
		clnt.sockfd = clntinfo.sockfd;
		clnt.portno = clntinfo.portno;
		memcpy(clnt.ipaddr,clntinfo.ipaddr,16);
		m_iLength = len;
	}
	virtual ~CTask(){}
};

class ThreadPool
{
private:
	static std::vector<CTask*> m_vecTaskList;	/* 任务列表 */
	static bool shutdown;						/* 线程池结束标识 */
	int m_iThreadNum;							/* 线程池启动的线程数 */
	pthread_t* pthread_id;

	static pthread_mutex_t m_pthreadMutex;		/* 线程同步锁 */
	static pthread_cond_t m_pthreadCond;		/* 线程同步的条件变量 */
	
	static pthread_mutex_t m_transMutex;		/* 套接字同步锁 */
	static bool m_bIsConn;						/* 转发是否连接 */
	static int m_iTransSock;					/* 转发套接字 */
protected:
	static void* ThreadFunc(void* arg);		/* 新线程的线程回调函数 */
public:
	ThreadPool(int threadnum = 10);			/* 默认开启线程数为10 */
	int AddTask(CTask* task);				/* 把任务加入到线程队列 */
	int StopThreadPool();					/* 使线程池中的线程退出 */
	int getTaskSize()						/* 获取当前任务队列的任务数 */
	{
		return m_vecTaskList.size();
	}

	void setTransInfo(int sock,bool isconn)
	{
		pthread_mutex_lock(&m_transMutex);
		m_iTransSock = sock;
		m_bIsConn = isconn;
		pthread_mutex_unlock(&m_transMutex);
	}
};

#endif
