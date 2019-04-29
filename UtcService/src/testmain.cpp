// =====================================================================================
//	Copyright (C) 2019 by Jiaxing Shao.All rights reserved.
//	文 件 名:  testmain.cpp
//	作    者:  Jiaxing Shao, 13315567369@163.com
//	版 本 号:  1.0
//	创建时间:  2019年03月12日 10时41分53秒
//	Compiler:  g++
//	描    述:  
// =====================================================================================

#include "tcpclient.h"

int main()
{
	TcpClient clnt;
	char ipaddr[16] = "192.168.136.1";
	int port = 10086;
	clnt.initialize(ipaddr, port);

	char buffer[1024] = { 0 };
	int length = 0;
	while(true)
	{
RecvLoop:
		length = clnt.recv_msg(buffer, 1024);
		if(length <= 0)
		{
			goto ReConnect;
		}
		length = clnt.send_msg(buffer, length);
		if(length <= 0)
		{
			goto ReConnect;
		}
	}

ReConnect:
	while(!clnt.getConnStat())
	{
		int ret = clnt.connect_serv();
		if(ret <= 0)
		{
			cout << "connect error, code: " << ret << endl;
			sleep(1);
		}
		else
		{
			cout << "connect success, code: " << ret << endl;
			break;
		}
	}
	goto RecvLoop;

	return 0;
}
