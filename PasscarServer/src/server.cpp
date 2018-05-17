#include "server.h"

Server::Server():m_threadpool(10)
{
}

Server::~Server()
{
}

void Server::EventSet(myevent_s* ev, int fd, void (*call_back)(int, int, void*, void*), void* arg)
{
	ev->fd = fd;
	ev->call_back = call_back;
	ev->events = 0;
	ev->arg = arg;
	ev->status = 0;
	ev->s_offset = 0;
	ev->last_active = time(NULL);
}

void Server::EventAdd(int epollFd, int events, myevent_s* ev)
{
	struct epoll_event epv = {0, {0}};
	int op;
	epv.data.ptr = ev;
	epv.events = ev->events = events;
	if(ev->status == 1)
	{
		op = EPOLL_CTL_MOD;
	}
	else
	{
		op = EPOLL_CTL_ADD;
		ev->status = 1;
	}

	int result = epoll_ctl(epollFd, op, ev->fd, &epv);

	if(result < 0)
	{
		g_logs.WriteLog("Event add failed[ fd = %d ],events[ %d ]", ev->fd, events);
	}
	else
	{
#ifndef NO_DEBUG
		g_logs.WriteLog("Event add succeed[fd=%d],op=%d,events[%0X]", ev->fd, op, events);
#endif
	}
}

void Server::EventDel(int epollFd, myevent_s* ev)
{
	struct epoll_event epv = {0, {0}};
	if(ev->status != 1)
		return;

	epv.data.ptr = ev;
	ev->status = 0;
	epoll_ctl(epollFd, EPOLL_CTL_DEL, ev->fd, &epv);
}

void Server::AcceptConn(int fd, int events, void* arg, void* self)
{
	Server* pthis = (Server*)self;
	struct sockaddr_in clnt_addr;
	socklen_t len = sizeof(sockaddr_in);
	memset(&clnt_addr, 0, len);
	int sock, i;

	sock = accept(fd, (struct sockaddr*)&clnt_addr, &len);
	if(sock == -1)
	{
		if(errno != EAGAIN && errno != EINTR)
		{
		}
		g_logs.WriteLog("%s:accept,%d",__func__, errno);
		return;
	}

	do
	{
		for(i = 0; i < MAX_EVENTS; i++)
		{
			if(pthis->m_events[i].status == 0)
			{
				break;
			}
		}

		if(i == MAX_EVENTS)
		{
			g_logs.WriteLog("%s:Max connection limit[%d].",__func__, MAX_EVENTS);
		}

		/*
		if(fcntl(sock, F_SETFL, O_NONBLOCK) < 0)
		{
			g_logs.WriteLog("%s:fnctl nonblocking failed[%d].", __func__, sock);
			break;
		}
		else
		{
			g_logs.WriteLog("Func %s:fnctl nonblocking succeed[%d].", __func__, sock);
		}
		*/

		pthis->EventSet(&pthis->m_events[i], sock, Server::RecvData, &pthis->m_events[i]);
		pthis->EventAdd(pthis->m_epollfd, EPOLLIN, &pthis->m_events[i]);
	}while(0);
	g_logs.WriteLog("New client %s connected on pos %d.", inet_ntoa(clnt_addr.sin_addr), i);
}

void Server::RecvData(int fd, int events, void* arg, void* self)
{
	Server* pthis = (Server*)self;
	struct myevent_s* ev = (struct myevent_s*)arg;
	int length;
	char buffer[BUF_SIZE] = {0};
	length = recv(fd, buffer, 4, 0);
	pthis->EventDel(pthis->m_epollfd, ev);
	if(length > 0)
	{
		if((buffer[0]&0xFF) == 0x44 && (buffer[1]&0xFF) == 0x46)
		{
			length = *(short*)(buffer+2);
			int len = recv(fd, buffer+4, length-4, 0);

			if(len == length-4)
			{
				CTask* task = new CTask;
				task->setData(buffer, length, ev->fd);
				pthis->m_threadpool.AddTask(task);
			}
			else
			{
#ifndef NO_DEBUG
				g_logs.WriteLog("Recv package error.");
#endif
			}
		}

		pthis->EventDel(pthis->m_epollfd, ev);
		pthis->EventSet(ev, fd, pthis->RecvData, ev);
		pthis->EventAdd(pthis->m_epollfd, EPOLLIN, ev);
	}
	else if(length == 0)
	{
		close(ev->fd);
#ifndef NO_DEBUG
		g_logs.WriteLog("Client pos %d disconnected.", ev - pthis->m_events);
#endif
	}
	else
	{
		close(ev->fd);
#ifndef NO_DEBUG
		g_logs.WriteLog("Recv[fd=%d] error[%d]:%s", fd, errno, strerror(errno));
#endif
	}
}

void Server::SendData(int fd, int events, void* arg, void* self)
{
	Server* pthis = (Server*)self;
	struct myevent_s* ev = (struct myevent_s*)arg;
	int len;

	char buffer[BUF_SIZE] = "Received.";
	int buflen = strlen(buffer);

	len = send(fd, buffer, buflen, 0);
	if(len > 0)
	{
		g_logs.WriteLog("返回应答消息成功.");
		pthis->EventDel(pthis->m_epollfd, ev);
		pthis->EventSet(ev, fd, pthis->RecvData,ev);
		pthis->EventAdd(pthis->m_epollfd, EPOLLIN, ev);
	}
	else if(len < 0)
	{
		close(ev->fd);
		pthis->EventDel(pthis->m_epollfd, ev);
#ifndef NO_DEBUG
		g_logs.WriteLog("Send[fd=%d] error[%d], len is %d", fd, errno, len);
#endif
	}
}

void Server::InitListen(char* port)
{
	int sock;
	sock = socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);
	if(sock == -1)
	{
		g_logs.WriteLog("socket() error, program exit!");
		exit(1);
	}

	int flag = 1;
	if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int)) == -1)
	{
		g_logs.WriteLog("setsockopt() error, program exit!");
		exit(1);
	}

	fcntl(sock, F_SETFL, O_NONBLOCK);	//set non_blocking

	EventSet(&m_events[MAX_EVENTS], sock, AcceptConn, &m_events[MAX_EVENTS]);
	EventAdd(m_epollfd, EPOLLIN, &m_events[MAX_EVENTS]);

	struct sockaddr_in serv_addr;
	memset(&serv_addr,0,sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(atoi(port));

	if(bind(sock,(struct sockaddr*)&serv_addr,sizeof(serv_addr)) == -1)
	{
		g_logs.WriteLog("bind() error, program exit!");
		exit(1);
	}

	if(listen(sock,5) == -1)
	{
		g_logs.WriteLog("listen() error, program exit!");
		exit(1);
	}
}

int Server::Start(char* port)
{
	m_epollfd = epoll_create(MAX_EVENTS);	//创建epoll文件描述符
	if(m_epollfd <= 0)
	{
		g_logs.WriteLog("Create epollfd failed[%d].", m_epollfd);
		return -1;
	}
	
	InitListen(port);
	struct epoll_event events[MAX_EVENTS];
	g_logs.WriteLog("Server running, %s 端口开启成功.", port);

	int checkpos = 0;

	while(true)
	{
		long now = time(NULL);
		for(int i = 0;i < 100;i++,checkpos++)
		{
			if(checkpos == MAX_EVENTS)
				checkpos = 0;

			if(m_events[checkpos].status != 1)
				continue;

			long duration = now - m_events[checkpos].last_active;
			if(duration >= 60)
			{	//长时间无操作则自动断开连接
				close(m_events[checkpos].fd);
				g_logs.WriteLog("连接长时间没有收到数据，主动断开连接.");
				EventDel(m_epollfd, &m_events[checkpos]);
			}
		}

		int fds = epoll_wait(m_epollfd, events, MAX_EVENTS, TIMEOUT);	//类似于select,返回值为可操作fd数量
		if(fds < 0)
		{
			g_logs.WriteLog("Epoll_wait error,exit.");
			break;
		}

		for(int i = 0; i < fds; i++)
		{
			myevent_s* ev = (struct myevent_s*)events[i].data.ptr;
			if((events[i].events&EPOLLIN) && (ev->events&EPOLLIN))
			{
				ev->call_back(ev->fd, events[i].events, ev->arg, this);
			}
			if((events[i].events&EPOLLOUT) && (ev->events&EPOLLOUT))
			{
				ev->call_back(ev->fd, events[i].events, ev->arg, this);
			}
		}
	}
	return 0;
}

