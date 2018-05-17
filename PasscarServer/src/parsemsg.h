#ifndef PARSEMESSAGE_H_H
#define PARSEMESSAGE_H_H

#include "defines.h"
#include "crc16.h"
#include "string.h"
#include "database.h"

class CTask
{
private:
	int parseMsg();		// 数据解析，并封装应答包
	void printMsg();	// 打印解析后数据
	int code_convert(string from_charset,string to_charset,char *inbuf,int inlen,char *outbuf,int outlen);
	VmsData data;		// 保存解析后数据
	int len;			// 原始报文长度
	char* buf;			// 原始报文
public:
	CTask();
	~CTask();
	int Run();

	void setData(char* buffer, int length, int sock)
	{
		m_sock = sock;
		buf = new char[length+1];
		buf[length] = '\0';
		memcpy(buf, buffer, length);
		len = length;
	}
public:
	int m_sock;		// 套接字
	char response[RESPONSE_SIZE];	// 应答报文
};

#endif
