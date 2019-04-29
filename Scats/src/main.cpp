// =====================================================================================
//	Copyright (C) 2018 by Jiaxing Shao.All rights reserved.
//	文 件 名:  main.cpp
//	作    者:  Jiaxing Shao, 13315567369@163.com
//	版 本 号:  1.0
//	创建时间:  2018年06月08日 11时52分38秒
//	Compiler:  g++
//	描    述:  
// =====================================================================================

#include <iostream>
using namespace std;
#include <stdlib.h>
#include <unistd.h>
#include "tcpclient.h"

int main(int argc, char *argv[])
{
	char szIpAddr[IP_SIZE] = {0};
	char szPort[PORT_SIZE] = {0};
	GetConfigureString("TcpServerIp",szIpAddr,IP_SIZE,"127.0.0.1",CONFFILE);
	GetConfigureString("TcpServerPort",szPort,PORT_SIZE,"10086",CONFFILE);

	TcpClient clnt;
	clnt.Start(szIpAddr, szPort);
	while(true)
	{
		sleep(1);
	}
	return EXIT_SUCCESS;
}
