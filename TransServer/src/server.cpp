#include "server.h"

extern pthread_mutex_t m_infoMutex;

BaseServer::BaseServer():m_threadpool(10)
{
	m_bIsConn = false;
	m_bIsConnWeb = false;
	m_bIsConnF5 = false;
	m_ServSocks = NULL;
	memset(m_ClntSocks,0,sizeof(ClntInfo)*MAX_CLIENT);
	memset(&m_ClntTransSock,0,sizeof(m_ClntTransSock));
	memset(&m_ServTransSock,0,sizeof(m_ServTransSock));
	memset(&m_ServWebSock,0,sizeof(m_ServWebSock));
	memset(&m_ClntWebSock,0,sizeof(m_ClntWebSock));
	memset(&m_ServF5Sock,0,sizeof(m_ServF5Sock));
	memset(&m_ClntF5Sock,0,sizeof(m_ClntF5Sock));
	FD_ZERO(&m_setServSock);
	FD_ZERO(&m_setClntSock);
	FD_ZERO(&m_setTransSock);
	FD_ZERO(&m_setWebSock);
	FD_ZERO(&m_setF5Sock);
	m_iServCount = 0;
	m_iClntCount = 0;
	m_iServMaxSock = 0;
	m_iClntMaxSock = 0;
	m_iTransMaxSock = 0;
	m_iWebMaxSock = 0;
	m_iF5MaxSock = 0;
	pthread_mutex_init(&m_mutxSock,NULL);
	pthread_mutex_init(&m_mutxTransSock,NULL);
}

BaseServer::~BaseServer()
{
	m_threadpool.StopThreadPool();
	pthread_mutex_destroy(&m_mutxSock);
	pthread_mutex_destroy(&m_mutxTransSock);
	if(m_ServSocks != NULL)
	{
		delete [] m_ServSocks;
		m_ServSocks = NULL;
	}
}

int BaseServer::sendn(const int sock,char * buf,int len,int flag)
{
	int length;
	length = send(sock,buf,len,flag);
	return length;
}

int BaseServer::recvn(const int sock,char * buf,int len,int flag)
{
	int length;
	length = recv(sock,buf,len,flag);  //第四个参数为0，则与read()功能一样
	return length;
}

int BaseServer::Listen(int port)
{
	int serv_sock;
	struct sockaddr_in serv_addr;
	serv_sock = socket(PF_INET,SOCK_STREAM,0);
	if(serv_sock == -1){
		close(serv_sock);
		return -1;
	}

	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 10;
	if(setsockopt(serv_sock,SOL_SOCKET,SO_SNDTIMEO,&timeout,sizeof(timeout)) == -1){
		close(serv_sock);
		return -2;
	}
	if(setsockopt(serv_sock,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout)) == -1){
		close(serv_sock);
		return -3;
	}
	int flag = 1;
	if(setsockopt(serv_sock,SOL_SOCKET,SO_REUSEADDR,&flag,sizeof(int)) == -1){
		close(serv_sock);
		return -4;
	}

	memset(&serv_addr,0,sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(port);
	
	if(bind(serv_sock,(struct sockaddr*)&serv_addr,sizeof(serv_addr)) == -1){
		close(serv_sock);
		return -5;
	}
	if(listen(serv_sock,5) == -1){
		close(serv_sock);
		return -6;
	}
	return serv_sock;
}

void BaseServer::StartListen(int minport,int maxport)
{
	m_iServCount = maxport-minport+1;
	if(m_iServCount > SERV_COUNT)
	{
		cout<<GetSystemTime()<<": 要求开启端口数超过限制，程序退出."<<endl;
		exit(1);
	}
	m_ServSocks = new ServInfo[m_iServCount];

	for(int i = minport;i <= maxport;i++)
	{
		m_ServSocks[i-minport].sockfd = Listen(i);	//套接字
		m_ServSocks[i-minport].portno = i;			//端口号
		FD_SET(m_ServSocks[i-minport].sockfd,&m_setServSock);
		if(m_ServSocks[i-minport].sockfd > 0)
		{
			if(m_ServSocks[i-minport].sockfd > m_iServMaxSock){
				m_iServMaxSock = m_ServSocks[i-minport].sockfd;
			}
			cout<<GetSystemTime()<<": "<<dec<<i<<" 端口开启成功."<<endl;
		}
		else
		{
			cout<<GetSystemTime()<<": "<<dec<<i<<" 端口开启失败，程序退出."<<endl;
			exit(1);
		}
	}
	cout<<GetSystemTime()<<": 全部端口开启完毕."<<endl;

	struct sockaddr_in clnt_addr;
	socklen_t clnt_addr_size = sizeof(clnt_addr);
	fd_set connfds;
	FD_ZERO(&connfds);
	int result = 0;
	struct timeval timeout;
	timeout.tv_sec = 5;
	timeout.tv_usec = 0;

	while(true)
	{
		memset(&clnt_addr,0,clnt_addr_size);
		connfds = m_setServSock;
		result = select(m_iServMaxSock+1,&connfds,NULL,NULL,&timeout);
		if(result > 0)
		{
			for(int j = 0;j < m_iServCount;j++)
			{
				if(FD_ISSET(m_ServSocks[j].sockfd,&connfds))
				{
					if(m_iClntCount < MAX_CLIENT)
					{
CONNECT:
						int clnt_sock;
						clnt_sock = accept(m_ServSocks[j].sockfd,(struct sockaddr*)&clnt_addr,&clnt_addr_size);
						if(clnt_sock > 0)
						{
							pthread_mutex_lock(&m_mutxSock);
							m_ClntSocks[m_iClntCount].sockfd = clnt_sock;
							m_ClntSocks[m_iClntCount].portno = m_ServSocks[j].portno;
							m_ClntSocks[m_iClntCount].lasttime = time(NULL);
							memcpy(m_ClntSocks[m_iClntCount].ipaddr,inet_ntoa(clnt_addr.sin_addr),16);
							m_iClntCount++;

							if(m_iClntMaxSock < clnt_sock)
								m_iClntMaxSock = clnt_sock;
							FD_SET(clnt_sock,&m_setClntSock);
#ifndef NO_DEBUG
							cout<<GetSystemTime()<<": Client "<<inet_ntoa(clnt_addr.sin_addr)<<" Connected on port "<<dec<<m_ClntSocks[m_iClntCount-1].portno<<endl;
#endif
							pthread_mutex_unlock(&m_mutxSock);
						}
					}
					else
					{
						pthread_mutex_lock(&m_mutxSock);
						for(int k = m_iClntCount-1;k >= 0;k--)
						{
							close(m_ClntSocks[k].sockfd);
						}
						FD_ZERO(&m_setClntSock);
						m_iClntCount = 0;
						pthread_mutex_unlock(&m_mutxSock);
						cout<<GetSystemTime()<<": Sockets are over the max sock of allowed("<<dec<<MAX_CLIENT<<") Disconnect all connections."<<endl;
						goto CONNECT;
					}
				}
			}
		}
		else if(result < 0)
		{
#ifndef NO_DEBUG
			cout<<GetSystemTime()<<": select() 发生错误!"<<endl;
#endif
		}
	}// while(true)
}

void BaseServer::StartWeb(int port)
{
	m_ServWebSock.sockfd = Listen(port);
	m_ServWebSock.portno = port;

	if(m_ServWebSock.sockfd > 0){
		FD_SET(m_ServWebSock.sockfd,&m_setWebSock);
		if(m_iWebMaxSock < m_ServWebSock.sockfd)
			m_iWebMaxSock = m_ServWebSock.sockfd;
		cout<<GetSystemTime()<<": Web交互端口 "<<dec<<port<<" 开启成功."<<endl;
	}
	else{
		cout<<GetSystemTime()<<": "<<port<<" 端口开启失败，程序退出."<<endl;
		exit(1);
	}

	int clnt_sock;
	struct sockaddr_in clnt_addr;
	socklen_t clnt_addr_size = sizeof(clnt_addr);
	memset(&clnt_addr,0,sizeof(clnt_addr));
	
	fd_set tempsets;
	struct timeval timeout;
	timeout.tv_sec = 5;
	timeout.tv_usec = 0;
	int result = 0;
	size_t buflen = MAXPACKLEN;
	char buf[MAXPACKLEN] = {0};
	
	while(true)
	{
		FD_ZERO(&tempsets);
		tempsets = m_setWebSock;
		result = select(m_iWebMaxSock+1,&tempsets,NULL,NULL,&timeout);
		if(result > 0)
		{
			if(FD_ISSET(m_ServWebSock.sockfd,&tempsets))
			{
				clnt_sock = accept(m_ServWebSock.sockfd,(struct sockaddr*)&clnt_addr,&clnt_addr_size);
				if(clnt_sock != -1)
				{
					if(m_bIsConnWeb){
						FD_CLR(m_ClntWebSock.sockfd,&m_setWebSock);
						close(m_ClntWebSock.sockfd);
					}
					m_ClntWebSock.sockfd = clnt_sock;
					FD_SET(m_ClntWebSock.sockfd,&m_setWebSock);
					if(m_iWebMaxSock < m_ClntWebSock.sockfd)
						m_iWebMaxSock = m_ClntWebSock.sockfd;
					m_ClntWebSock.portno = m_ServWebSock.portno;
					memcpy(m_ClntWebSock.ipaddr,inet_ntoa(clnt_addr.sin_addr),16);
					m_bIsConnWeb = true;
#ifndef NO_DEBUG
					cout<<GetSystemTime()<<": Web Client Connected "<<m_ClntWebSock.ipaddr<<endl;
#endif
				}
			}
			if(m_bIsConnWeb)
			{
				if(FD_ISSET(m_ClntWebSock.sockfd,&tempsets))
				{
					while(true)
					{
						memset(buf,0,buflen);
						int len = recvn(m_ClntWebSock.sockfd,buf,buflen);
						
						if(len == 6)
						{
							if((buf[0]&0xFF) == 0xAA && (buf[1]&0xFF) == 0xAA)
							{
								/********************添加获取站点信息及截止时间功能***************************/
								g_logs.WriteLog("收到采集数据截止日期更新命令.");
								pthread_mutex_lock(&m_infoMutex);
								iTimeSpace = GetConfigureFromDatabase();
								if(iTimeSpace == -1)
								{
									char space[4] = {0};
#ifndef NO_DEBUG									
									cout<<"获取数据库截止日期配置失败，从本地读取"<<endl;
#endif
									g_logs.WriteLog("获取数据库截止日期配置失败，从本地读取.");
									GetConfigureString("TIMESPACE",space,4,"30");
									iTimeSpace = atoi(space);
								}
								pthread_mutex_unlock(&m_infoMutex);
								cout<<"超过此截止天数的数据不再接收："<<dec<<iTimeSpace<<" 天"<<endl;
								g_logs.WriteLog("超过此截止天数的数据不再接收：%d",iTimeSpace);
								/***************************************************** */
							}
						}
						else if(len == 0)
						{
							FD_CLR(m_ClntWebSock.sockfd,&m_setWebSock);
							close(m_ClntWebSock.sockfd);
							cout<<GetSystemTime()<<": Web Client DisConnect "<<m_ClntWebSock.ipaddr<<endl;
							m_bIsConnWeb = false;
							break;
						}
						else if(len < 0)
						{
							break;
						}
					}
				}
			}
		}
		else if(result < 0)
		{
			cout<<GetSystemTime()<<": Web select() error!"<<endl;
		}
	}
}

void BaseServer::StartF5(int port)
{
	m_ServF5Sock.sockfd = Listen(port);
	m_ServF5Sock.portno = port;

	if(m_ServF5Sock.sockfd > 0){
		FD_SET(m_ServF5Sock.sockfd,&m_setF5Sock);
		if(m_iF5MaxSock < m_ServF5Sock.sockfd)
			m_iF5MaxSock = m_ServF5Sock.sockfd;
		cout<<GetSystemTime()<<": F5交互端口 "<<dec<<port<<" 开启成功."<<endl;
	}
	else{
		cout<<GetSystemTime()<<": "<<port<<" 端口开启失败，程序退出."<<endl;
		exit(1);
	}

	int clnt_sock;
	struct sockaddr_in clnt_addr;
	socklen_t clnt_addr_size = sizeof(clnt_addr);
	memset(&clnt_addr,0,sizeof(clnt_addr));
	
	fd_set tempsets;
	struct timeval timeout;
	timeout.tv_sec = 5;
	timeout.tv_usec = 0;
	int result = 0;
	size_t buflen = MAXPACKLEN;
	char buf[MAXPACKLEN] = {0};
	
	while(true)
	{
		FD_ZERO(&tempsets);
		tempsets = m_setF5Sock;
		result = select(m_iF5MaxSock+1,&tempsets,NULL,NULL,&timeout);
		if(result > 0)
		{
			if(FD_ISSET(m_ServF5Sock.sockfd,&tempsets))
			{
				clnt_sock = accept(m_ServF5Sock.sockfd,(struct sockaddr*)&clnt_addr,&clnt_addr_size);
				if(clnt_sock != -1)
				{
					if(m_bIsConnF5){
						FD_CLR(m_ClntF5Sock.sockfd,&m_setF5Sock);
						close(m_ClntF5Sock.sockfd);
					}
					m_ClntF5Sock.sockfd = clnt_sock;
					FD_SET(m_ClntF5Sock.sockfd,&m_setF5Sock);
					if(m_iF5MaxSock < m_ClntF5Sock.sockfd)
						m_iF5MaxSock = m_ClntF5Sock.sockfd;
					m_ClntF5Sock.portno = m_ServF5Sock.portno;
					memcpy(m_ClntF5Sock.ipaddr,inet_ntoa(clnt_addr.sin_addr),16);
					m_bIsConnF5 = true;
#ifndef NO_DEBUG
					cout<<GetSystemTime()<<": F5 Client Connected "<<m_ClntF5Sock.ipaddr<<endl;
#endif
				}
			}
			if(m_bIsConnF5)
			{
				if(FD_ISSET(m_ClntF5Sock.sockfd,&tempsets))
				{
					while(true)
					{
						int len = recvn(m_ClntF5Sock.sockfd,buf,buflen);
						memset(buf,0,buflen);
						if(len == 0){
							FD_CLR(m_ClntF5Sock.sockfd,&m_setF5Sock);
							close(m_ClntF5Sock.sockfd);
							m_ClntF5Sock.sockfd = 0;
#ifndef NO_DEBUG
							cout<<GetSystemTime()<<": F5 Client DisConnect "<<m_ClntF5Sock.ipaddr<<endl;
#endif
							m_bIsConnF5 = false;
							break;
						}
						else if(len < 0){
							break;
						}
					}
				}
			}
		}//end of if(result > 0)
		else if(result < 0)
		{
			cout<<GetSystemTime()<<": F5 select() error!"<<endl;
		}
	}//end of while
}

void BaseServer::StartTrans(int port)
{
	m_ServTransSock.sockfd = Listen(port);
	m_ServTransSock.portno = port;

	if(m_ServTransSock.sockfd > 0){
		FD_SET(m_ServTransSock.sockfd,&m_setTransSock);
		if(m_iTransMaxSock < m_ServTransSock.sockfd)
			m_iTransMaxSock = m_ServTransSock.sockfd;
		cout<<GetSystemTime()<<": 转发端口 "<<dec<<port<<" 开启成功."<<endl;
	}
	else{
		cout<<GetSystemTime()<<": "<<port<<" 端口开启失败，程序退出."<<endl;
		exit(1);
	}

	int clnt_sock;
	struct sockaddr_in clnt_addr;
	socklen_t clnt_addr_size = sizeof(clnt_addr);
	memset(&clnt_addr,0,sizeof(clnt_addr));
	
	fd_set tempsets;
	struct timeval timeout;
	timeout.tv_sec = 5;
	timeout.tv_usec = 0;
	int result = 0;
	size_t buflen = MAXPACKLEN;
	char buf[MAXPACKLEN] = {0};
	
	while(true)
	{
		FD_ZERO(&tempsets);
		tempsets = m_setTransSock;
		result = select(m_iTransMaxSock+1,&tempsets,NULL,NULL,&timeout);
		if(result > 0)
		{
			if(FD_ISSET(m_ServTransSock.sockfd,&tempsets))
			{
				clnt_sock = accept(m_ServTransSock.sockfd,(struct sockaddr*)&clnt_addr,&clnt_addr_size);
				if(clnt_sock != -1)
				{
					pthread_mutex_lock(&m_mutxTransSock);
					if(m_bIsConn){
						FD_CLR(m_ClntTransSock.sockfd,&m_setTransSock);
						close(m_ClntTransSock.sockfd);
					}
					m_ClntTransSock.sockfd = clnt_sock;
					FD_SET(m_ClntTransSock.sockfd,&m_setTransSock);
					if(m_iTransMaxSock < m_ClntTransSock.sockfd)
						m_iTransMaxSock = m_ClntTransSock.sockfd;
					m_ClntTransSock.portno = m_ServTransSock.portno;
					memcpy(m_ClntTransSock.ipaddr,inet_ntoa(clnt_addr.sin_addr),16);
					m_bIsConn = true;
					m_threadpool.setTransInfo(m_ClntTransSock.sockfd,m_bIsConn);
					pthread_mutex_unlock(&m_mutxTransSock);
#ifndef NO_DEBUG
					cout<<GetSystemTime()<<": Trans Client Connected "<<m_ClntTransSock.ipaddr<<endl;
#endif
				}
			}
			if(m_bIsConn)
			{
				if(FD_ISSET(m_ClntTransSock.sockfd,&tempsets))
				{
					while(true)
					{
						pthread_mutex_lock(&m_mutxTransSock);	/*  */
						int len = recvn(m_ClntTransSock.sockfd,buf,buflen);
						memset(buf,0,buflen);
						if(len == 0)
						{
							FD_CLR(m_ClntTransSock.sockfd,&m_setTransSock);
							close(m_ClntTransSock.sockfd);
#ifndef NO_DEBUG
							cout<<GetSystemTime()<<": Trans Client DisConnect "<<m_ClntTransSock.ipaddr<<endl;
#endif
							m_bIsConn = false;
							m_threadpool.setTransInfo(0,m_bIsConn);
							break;
						}
						else if(len < 0)
						{
							break;
						}
					}
					pthread_mutex_unlock(&m_mutxTransSock);
				}
			}
		}
		else if(result < 0)
		{
#ifndef NO_DEBUG
			cout<<GetSystemTime()<<": Trans select() error!"<<endl;
#endif
		}
	}//end of while
}

void BaseServer::StartRecv()
{
	fd_set recvfds;
	int result = 0;
	struct timeval timeout;
	timeout.tv_sec = 5;
	timeout.tv_usec = 0;

	while(1)
	{
		FD_ZERO(&recvfds);
		recvfds = m_setClntSock;
		result = select(m_iClntMaxSock+1,&recvfds,NULL,NULL,&timeout);
		if(result > 0)
		{
			pthread_mutex_lock(&m_mutxSock);
			for(int i=0;i<m_iClntCount;i++)
			{
				if(FD_ISSET(m_ClntSocks[i].sockfd,&recvfds))
				{
					m_ClntSocks[i].lasttime = time(NULL);
					char* buf = new char[MAXPACKLEN];
					memset(buf,0,MAXPACKLEN);
					int len = recvn(m_ClntSocks[i].sockfd,buf,4);
					if(len == 0)
					{
ClntDisConnect:
						ClntInfo temp;
						temp.sockfd = m_ClntSocks[i].sockfd;
						memcpy(temp.ipaddr,m_ClntSocks[i].ipaddr,16);
						FD_CLR(m_ClntSocks[i].sockfd,&m_setClntSock);

						for(int j = i;j < m_iClntCount-1;j++)
						{
							m_ClntSocks[j].sockfd = m_ClntSocks[j+1].sockfd;
							memcpy(m_ClntSocks[j].ipaddr,m_ClntSocks[j+1].ipaddr,16);
							m_ClntSocks[j].portno = m_ClntSocks[j+1].portno;
							m_ClntSocks[j].lasttime = m_ClntSocks[j+1].lasttime;
						}
						close(temp.sockfd);
						m_iClntCount--;
						i-=1;
#ifndef NO_DEBUG
						cout<<GetSystemTime()<<": Client Disconnected "<<temp.ipaddr<<endl;
#endif
					}
					else if(len > 0)
					{
						if(buf[0] == 0x44 && buf[1] == 0x46)
						{
							int buflen = *((int*)(buf+2));
							if(buflen > MAXPACKLEN-4)
							{
								cout<<GetSystemTime()<<": 数据包长超过最大值("<<dec<<MAXPACKLEN<<")."<<endl;
								g_logs.WriteLog(": 数据包长超过最大值(%d).",MAXPACKLEN);
							}
							len = recvn(m_ClntSocks[i].sockfd,buf+4,buflen+4);
							if(len > 0)
							{
								/* 加入任务到线程池,剩下的操作由线程池处理 */
								MyCTask* task = new MyCTask;
								task->setData(buf,m_ClntSocks[i],buflen+8);
#ifndef NO_DEBUG
								ostringstream temposs;
								temposs<<setfill('0')<<uppercase;
								for(int ii=0;ii<buflen+8;ii++)
								{
									temposs<<setw(2)<<hex<<static_cast<int>(buf[ii] & 0xFF)<<' ';
								}
								g_logs.WriteLog(" 已接收到报文(未解析) len:%d, message:%s",buflen+8,temposs.str().c_str());
#endif
								m_threadpool.AddTask(task);
							}
							else if(len == 0)
							{
								goto ClntDisConnect;
							}
							else
							{
#ifndef NO_DEBUG
								cout<<GetSystemTime()<<": Second recvn() error!"<<endl;
#endif
							}
						}
						else
						{
							ostringstream ossHead;
							ossHead<<setfill('0')<<uppercase;
							for(int k=0;k < len;k++)
							{
								ossHead << setw(2) << hex << static_cast<int>(buf[k]&0xFF) <<' ';
							}
							g_logs.WriteLog(": 包头错误:%s",ossHead.str().c_str());
						}
					}
					else
					{
#ifndef NO_DEBUG
						cout<<GetSystemTime()<<": First recvn() error!"<<endl;
#endif
					}
				}
				else
				{
					if(time(NULL) - m_ClntSocks[i].lasttime > TIMEOUT)
					{
						int sockfd;
						char tempaddr[16] = {0};
						sockfd = m_ClntSocks[i].sockfd;
						memcpy(tempaddr, m_ClntSocks[i].ipaddr, 16);
						FD_CLR(m_ClntSocks[i].sockfd,&m_setClntSock);

						for(int j = i;j < m_iClntCount-1;j++)
						{
							m_ClntSocks[j].sockfd = m_ClntSocks[j+1].sockfd;
							memset(m_ClntSocks[j].ipaddr, 0, 16);
							memcpy(m_ClntSocks[j].ipaddr, m_ClntSocks[j+1].ipaddr, 16);
							m_ClntSocks[j].portno = m_ClntSocks[j+1].portno;
							m_ClntSocks[j].lasttime = m_ClntSocks[j+1].lasttime;
						}
						close(sockfd);
						m_iClntCount--;
						i-=1;
						g_logs.WriteLog("%s 长时间无操作，主动断开连接.", tempaddr);
					}
				}
			}
			pthread_mutex_unlock(&m_mutxSock);
		}
		else if(result < 0)
		{
#ifndef NO_DEBUG
			cout<<GetSystemTime()<<":Recv select() error!"<<endl;
#endif
		}
	}//end of while
}

void* BaseServer::pthread_receive(void* arg)
{
	BaseServer* pthis = (BaseServer*)arg;
	pthis->StartRecv();
	return NULL;
}

void* BaseServer::pthread_accept(void* arg)
{
	BaseServer* pthis = (BaseServer*)arg;
	pthis->StartListen(13301,13330);
	return NULL;
}

void* BaseServer::pthread_trans(void* arg)
{
	BaseServer* pthis = (BaseServer*)arg;
	pthis->StartTrans(13334);
	return NULL;
}

void* BaseServer::pthread_web(void* arg)
{
	BaseServer* pthis = (BaseServer*)arg;
	pthis->StartWeb(10086);
	return NULL;
}

void* BaseServer::pthread_F5(void* arg)
{
	BaseServer* pthis = (BaseServer*)arg;
	pthis->StartF5(13335);
	return NULL;
}

void BaseServer::Start()
{
	pthread_t tid[5];
	pthread_create(&tid[0],NULL,pthread_F5,this);
	pthread_detach(tid[0]);
	pthread_create(&tid[1],NULL,pthread_receive,this);
	pthread_detach(tid[1]);
	pthread_create(&tid[2],NULL,pthread_accept,this);
	pthread_detach(tid[2]);
	pthread_create(&tid[3],NULL,pthread_trans,this);
	pthread_detach(tid[3]);
	pthread_create(&tid[4],NULL,pthread_web,this);
	pthread_detach(tid[4]);
}

/********************************************************************** MyCTask *********************************************************************************/

int MyCTask::Run()
{
	JdMessage jd;
	stuOutMessage outMsg;
	stuLogInfo loginfo;
	memset(&loginfo,0,sizeof(stuLogInfo));
	memset(&outMsg,0,sizeof(stuOutMessage));
	void* data = NULL;

	int16_t answer = jd.ParseMessage(m_ptrData,m_iLength,outMsg,data);

	ostringstream ossMsg;
	ossMsg << setfill('0')<<uppercase;
	for(int i=0;i<m_iLength;i++)
	{
		ossMsg << setw(2) << hex << static_cast<int>(m_ptrData[i] & 0XFF);
	}
	switch(outMsg.command_type)
	{	// 日志组包
		case 1:
			if(data != NULL)
			{
				memcpy(loginfo.devcode,((stuInTraffic*)data)->devcode,17);
				memcpy(loginfo.stationcode,((stuInTraffic*)data)->stationcode,16);
				memcpy(loginfo.createtime,((stuInTraffic*)data)->record_time,32);
				memcpy(loginfo.servertime,((stuInTraffic*)data)->receive_time,32);
				loginfo.isretried = (int)outMsg.is_retried;
				loginfo.t_id = ((stuInTraffic*)data)->t_id;
				loginfo.s_id = ((stuInTraffic*)data)->s_id;
			}
			memcpy(loginfo.remote_ip,clnt.ipaddr,16);
			loginfo.msglen = m_iLength;
			loginfo.messagetype = 1;
			loginfo.parseresult = answer;
			memcpy(loginfo.sourcemsg,ossMsg.str().c_str(),m_iLength*2);
			break;
		case 57:
			if(data != NULL)
			{
				memcpy(loginfo.devcode,((stuInRealtime*)data)->devcode,17);
				memcpy(loginfo.stationcode,((stuInRealtime*)data)->stationcode,16);
				memcpy(loginfo.createtime,((stuInRealtime*)data)->record_time,32);
				memcpy(loginfo.servertime,((stuInRealtime*)data)->receive_time,32);
				loginfo.isretried = (int)outMsg.is_retried;
				loginfo.t_id = ((stuInRealtime*)data)->t_id;
				loginfo.s_id = ((stuInRealtime*)data)->s_id;
			}
			memcpy(loginfo.remote_ip,clnt.ipaddr,16);
			loginfo.msglen = m_iLength;
			loginfo.messagetype = 57;
			loginfo.parseresult = answer;
			memcpy(loginfo.sourcemsg,ossMsg.str().c_str(),m_iLength*2);
			break;
		case 97:
			if(data != NULL)
			{
				memcpy(loginfo.devcode,((stuInWeight*)data)->devcode,17);
				memcpy(loginfo.stationcode,((stuInWeight*)data)->stationcode,16);
				memcpy(loginfo.createtime,((stuInWeight*)data)->record_time,32);
				memcpy(loginfo.servertime,((stuInWeight*)data)->receive_time,32);
				loginfo.isretried = (int)outMsg.is_retried;
				loginfo.t_id = ((stuInWeight*)data)->t_id;
				loginfo.s_id = ((stuInWeight*)data)->s_id;
			}
			memcpy(loginfo.remote_ip,clnt.ipaddr,16);
			loginfo.msglen = m_iLength;
			loginfo.messagetype = 97;
			loginfo.parseresult = answer;
			memcpy(loginfo.sourcemsg,ossMsg.str().c_str(),m_iLength*2);
			break;
		case 98:
			if(data != NULL)
			{
				memcpy(loginfo.devcode,((stuInAxlNumber*)data)->devcode,17);
				memcpy(loginfo.stationcode,((stuInAxlNumber*)data)->stationcode,16);
				memcpy(loginfo.createtime,((stuInAxlNumber*)data)->record_time,32);
				memcpy(loginfo.servertime,((stuInAxlNumber*)data)->receive_time,32);
				loginfo.isretried = (int)outMsg.is_retried;
				loginfo.t_id = ((stuInAxlNumber*)data)->t_id;
				loginfo.s_id = ((stuInAxlNumber*)data)->s_id;
			}
			memcpy(loginfo.remote_ip,clnt.ipaddr,16);
			loginfo.msglen = m_iLength;
			loginfo.messagetype = 98;
			loginfo.parseresult = answer;
			memcpy(loginfo.sourcemsg,ossMsg.str().c_str(),m_iLength*2);
			break;
		case 104:
			if(data != NULL)
			{
				memcpy(loginfo.devcode,((stuInAxlWeight*)data)->devcode,17);
				memcpy(loginfo.stationcode,((stuInAxlWeight*)data)->stationcode,16);
				memcpy(loginfo.createtime,((stuInAxlWeight*)data)->record_time,32);
				memcpy(loginfo.servertime,((stuInAxlWeight*)data)->receive_time,32);
				loginfo.isretried = (int)outMsg.is_retried;
				loginfo.t_id = ((stuInAxlWeight*)data)->t_id;
				loginfo.s_id = ((stuInAxlWeight*)data)->s_id;
			}
			memcpy(loginfo.remote_ip,clnt.ipaddr,16);
			loginfo.msglen = m_iLength;
			loginfo.messagetype = 104;
			loginfo.parseresult = answer;
			memcpy(loginfo.sourcemsg,ossMsg.str().c_str(),m_iLength*2);
			break;
		case 65:
			if(data != NULL)
			{
				memcpy(loginfo.devcode,((stuInExceed*)data)->devcode,17);
				memcpy(loginfo.stationcode,((stuInExceed*)data)->stationcode,16);
				memcpy(loginfo.createtime,((stuInExceed*)data)->recordtime,32);
				memcpy(loginfo.servertime,((stuInExceed*)data)->servertime,32);
				loginfo.isretried = (int)outMsg.is_retried;
				loginfo.t_id = ((stuInExceed*)data)->t_id;
				loginfo.s_id = ((stuInExceed*)data)->s_id;
			}
			memcpy(loginfo.remote_ip,clnt.ipaddr,16);
			loginfo.msglen = m_iLength;
			loginfo.messagetype = 65;
			loginfo.parseresult = answer;
			memcpy(loginfo.sourcemsg,ossMsg.str().c_str(),m_iLength*2);
			break;
		default:
			memcpy(loginfo.remote_ip,clnt.ipaddr,16);
			loginfo.msglen = m_iLength;
			loginfo.messagetype = outMsg.command_type;
			loginfo.parseresult = answer;
			memcpy(loginfo.sourcemsg,ossMsg.str().c_str(),m_iLength*2);
			g_logs.WriteLog("No this commandtype:%d",outMsg.command_type);
	}

	bool ret = false;
	type = outMsg.command_type;
	switch(outMsg.command_type)
	{
		case 1:
			if(answer == 17)
			{
				if(data != NULL){
					ret = insert_traffic_msg((stuInTraffic*)data);
				}
				if(!ret){
					answer = oracle_fail;
					cout<<GetSystemTime()<<": 交调数据插入数据库失败."<<endl;
					g_logs.WriteLog("交调数据插入数据库失败.");
				}
				else
				{
#ifndef NO_DEBUG
					cout<<GetSystemTime()<<": 交调数据解析成功，入库成功."<<endl;
#endif
					g_logs.WriteLog("交调数据解析成功，入库成功.");
				}
			}
			else
			{
				cout<<GetSystemTime()<<": 交调数据解析失败，parseresult(结果)："<<hex<<answer<<endl;
				g_logs.WriteLog("交调数据解析失败，parseresult(结果)：%x",answer);
				jd.PrintMessage(&loginfo,255);
			}
			if(data != NULL)
			{
				delete (stuInTraffic*)data;
				data = NULL;
			}
			break;
		case 57:
			if(answer == 17)
			{
				if(data != NULL)
					ret = insert_realtime_msg((stuInRealtime*)data);
				if(!ret){
					answer = oracle_fail;
					cout<<GetSystemTime()<<": 单车数据插入数据库失败."<<endl;
					g_logs.WriteLog("单车数据插入数据库失败.");
				}
				else
				{
#ifndef NO_DEBUG
					cout<<GetSystemTime()<<": 单车数据解析成功，入库成功."<<endl;
#endif
					g_logs.WriteLog("单车数据解析成功，入库成功.");
				}
			}
			else
			{
				cout<<GetSystemTime()<<": 单车数据解析失败，parseresult(结果)："<<hex<<answer<<endl;
				g_logs.WriteLog("单车数据解析失败，parseresult(结果)：%x",answer);
				jd.PrintMessage(&loginfo,255);
			}
			if(data != NULL)
			{
				delete (stuInRealtime*)data;
				data = NULL;
			}
			break;
		case 97:
			if(answer == 17)
			{
				if(data != NULL)
					ret = insert_weight_msg((stuInWeight*)data);
				if(!ret){
					answer = oracle_fail;
					cout<<GetSystemTime()<<": 车重数据插入数据库失败."<<endl;
				}
				else
				{
#ifndef NO_DEBUG
					cout<<GetSystemTime()<<": 车重数据解析成功，入库成功."<<endl;
#endif
					g_logs.WriteLog("车重数据解析成功，入库成功.");
				}
			}
			else
			{
				cout<<GetSystemTime()<<": 车重数据解析失败，parseresult(结果)："<<hex<<answer<<endl;
				g_logs.WriteLog("车重数据解析失败，parseresult(结果)：%x",answer);
				jd.PrintMessage(&loginfo,255);
			}
			if(data != NULL)
			{
				delete (stuInWeight*)data;
				data = NULL;
			}
			break;
		case 98:
			if(answer == 17)
			{
				if(data != NULL)
					ret = insert_axlnumber_msg((stuInAxlNumber*)data);
				if(!ret){
					answer = oracle_fail;
					cout<<GetSystemTime()<<": 轴数数据插入数据库失败."<<endl;
					g_logs.WriteLog("轴数数据插入数据库失败.");
				}
				else
				{
#ifndef NO_DEBUG
					cout<<GetSystemTime()<<": 轴数数据解析成功，入库成功."<<endl;
#endif
					g_logs.WriteLog("轴数数据解析成功，入库成功.");
				}
			}
			else
			{
				cout<<GetSystemTime()<<": 轴数数据解析失败，parseresult(结果)："<<hex<<answer<<endl;
				g_logs.WriteLog("轴数数据解析失败，parseresult(结果)：%x",answer);
				jd.PrintMessage(&loginfo,255);
			}
			if(data != NULL)
			{
				delete (stuInAxlNumber*)data;
				data = NULL;
			}
			break;
		case 104:
			if(answer == 17)
			{
				if(data != NULL)
					ret = insert_axlweight_msg((stuInAxlWeight*)data);
				if(!ret){
					answer = oracle_fail;
					cout<<GetSystemTime()<<": 轴重数据插入数据库失败."<<endl;
				}
				else
				{
#ifndef NO_DEBUG
					cout<<GetSystemTime()<<": 轴重数据解析成功，入库成功."<<endl;
#endif
					g_logs.WriteLog("轴重数据解析成功，入库成功.");
				}
			}
			else
			{
				cout<<GetSystemTime()<<": 轴重数据解析失败，parseresult(结果)："<<hex<<answer<<endl;
				g_logs.WriteLog("轴重数据解析失败，parseresult(结果)：%x",answer);
				jd.PrintMessage(&loginfo,255);
			}
			if(data != NULL)
			{
				delete (stuInAxlWeight*)data;
				data = NULL;
			}
			break;
		case 65:
			if(answer == 17)
			{
				if(data != NULL)
					ret = insert_exceed_msg((stuInExceed*)data);
				if(!ret){
					answer = oracle_fail;
					cout<<GetSystemTime()<<": 劝返数据插入数据库失败."<<endl;
					g_logs.WriteLog("劝返数据插入数据库失败.");
				}
				else
				{
#ifndef NO_DEBUG
					cout<<GetSystemTime()<<": 劝返数据解析成功，入库成功."<<endl;
#endif
					g_logs.WriteLog("劝返数据解析成功，入库成功.");
				}
			}
			else
			{
				cout<<GetSystemTime()<<": 劝返数据解析失败，parseresult(结果)："<<hex<<answer<<endl;
				g_logs.WriteLog("劝返数据解析失败，parseresult(结果)：%x",answer);
				jd.PrintMessage(&loginfo,255);
			}
			if(data != NULL)
			{
				delete (stuInExceed*)data;
				data = NULL;
			}
			break;
		default:
			g_logs.WriteLog("No this commandtype:%d",outMsg.command_type);
	}
WriteLog:
	/* 返回应答,并写入日志 */
	if(answer == 0x10 || answer == 0xF0 || answer == 0xA0 || answer == 0x52)
	{
		insert_error_log(loginfo);
	}
	else
	{
		if(answer == 0xF4)
			answer = 0x11;
		char answer_msg[32] = {0};
		jd.AnswerMessage(answer_msg,m_ptrData,answer);
		int n = send(clnt.sockfd,answer_msg,18,0);
		if(n < 0)
		{
#ifndef NO_DEBUG
			cout << GetSystemTime() << ": send() error!" << endl;
#endif
		}
		insert_recv_log(loginfo);
	}
	return answer;
}

