#include <iostream>
#include <string.h>
using namespace std;
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "log.h"
#include "threadpool.h"

#ifndef SERVER_H_H
#define SERVER_H_H

struct myevent_s
{
	int fd;				//套接字
	void (*call_back)(int fd, int events,void* arg, void* self);
	int events;			//记录事件
	void* arg;			//myevent_s*
	int status;			//1: in epoll wait list, 0: not in
	int s_offset;
	long last_active;	//last active time
};

class Server
{
public:
	Server();
	~Server();

	void EventSet(myevent_s* ev, int fd, void (*call_back)(int, int, void*, void*), void* arg);
	void EventAdd(int epollFd, int events, myevent_s* ev);
	void EventDel(int epollFd, myevent_s* ev);
public:
	static void RecvData(int fd, int events, void* arg, void* self);
	static void SendData(int fd, int events, void* arg, void* self);
	static void AcceptConn(int fd, int events, void* arg, void* self);

	void InitListen(char* port);
	int Start(char* port);
private:
	int m_epollfd;
	myevent_s m_events[MAX_EVENTS+1];	//最后一个保存的是server套接字的myevent_s
	ThreadPool m_threadpool;

	pthread_mutex_t m_mutxSock;
};

#endif
