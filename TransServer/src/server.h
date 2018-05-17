//--------------------------------------------------------------
// Time		:2016-12-14
// Author	:邵佳兴
// Effect	:创建server服务端，监听client的连接，开启转发端口，转发端口只接受一个连接
//--------------------------------------------------------------

#ifndef _SERVER_H_H
#define _SERVER_H_H

#include "log.h"
#include "pthreadpool.h"
#include "jdmessage.h"
#include "database.h"

class MyCTask:public CTask
{
public:
	int Run();
	int Send(int sock)
	{
		int result = 1;
		result = send(sock,m_ptrData,m_iLength,0);
		return result;
	}
	~MyCTask()
	{
		if(m_ptrData != NULL)
		{
			delete [] m_ptrData;
			m_ptrData = NULL;
		}
	}
};

/* 当 send 失败时利用信号来变更连接状态 */
class BaseServer
{
private:
	int recvn(const int sock,char * buf,int len,int flag = 0);
	int sendn(const int sock,char * buf,int len,int flag = 0);
	int Listen(int port);
protected:
	int m_iServCount;
	int m_iClntCount;
	int m_iClntMaxSock;
	int m_iServMaxSock;
	int m_iTransMaxSock;
	int m_iWebMaxSock;
	int m_iF5MaxSock;

	bool m_bIsConn;					//判断转发是否有连接
	bool m_bIsConnWeb;
	bool m_bIsConnF5;

	ClntInfo m_ClntSocks[MAX_CLIENT];
	ClntInfo m_ClntTransSock;
	ClntInfo m_ClntWebSock;
	ClntInfo m_ClntF5Sock;

	ServInfo *m_ServSocks;
	ServInfo m_ServTransSock;
	ServInfo m_ServWebSock;
	ServInfo m_ServF5Sock;

	fd_set m_setServSock;
	fd_set m_setClntSock;
	fd_set m_setTransSock;
	fd_set m_setWebSock;
	fd_set m_setF5Sock;

	pthread_mutex_t m_mutxSock;
	pthread_mutex_t m_mutxTransSock;

	ThreadPool m_threadpool;		//线程池,处理数据并转发
protected:
	void StartListen(int minport,int maxport);	//开启采集监听
	void StartTrans(int port);		//开启转发任务
	void StartRecv();				//开始接收数据
	void StartWeb(int port);		//开启与应用层对接任务
	void StartF5(int port);			//开启负载专用ping端口
	static void* pthread_receive(void* arg);
	static void* pthread_accept(void* arg);
	static void* pthread_trans(void* arg);		//转发
	static void* pthread_web(void* arg);		//应用层对接
	static void* pthread_F5(void* arg);			//负载均衡
public:
	BaseServer();
	virtual ~BaseServer();
	virtual void Start();
};

#endif
