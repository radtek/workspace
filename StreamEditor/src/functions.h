//--------------------------------------------------------------
// Time		:2016-12-14
// Author	:JiaXing Shao
// Effect	:
// Function :
//--------------------------------------------------------------

#ifndef _FUNCTIONS_H_H_H
#define _FUNCTIONS_H_H_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <list>
using namespace std;
#include <stdio.h>
#include <time.h>

#ifdef __GNUC__
#include <string.h>
#include <iconv.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <fcntl.h>
#else
#include <string>
#endif

#ifdef __GNUC__
#define CONFFILE "./Configure.ini"
#else
#define CONFFILE ".\\Configure.ini"
#endif

/* * * * * * * * * * * * * * * * * * * */
typedef unsigned char 		uint8;
typedef unsigned short 		uint16;
typedef unsigned int 		uint32;
typedef unsigned long long 	uint64;
/* * * * * * * * * * * * * * * * * * * */
typedef struct
{
	int begin;
	int end;
}t_position;

// 获取配置信息
void GetConfigureString(string value_name, char *value_buf, unsigned int strlen, string default_value, const char *filename);
// 返回数据的16进制字符串
string ParseMessageHex(const unsigned char *src, unsigned int srclen);
// 返回当前时间
string GetSystemTime();
// 数字转字符串，模板
template<typename T> string ToString(const T &n)
{
	ostringstream oss;
	oss << n;
	string str = oss.str();
	return str;
}

#ifdef __GNUC__
// 转换字符集,utf-8转未gb2312
int Utf8ToGb2312(char *inbuf, size_t inlen, char *outbuf, size_t outlen);
// 线程休眠,毫秒
void Sleep(unsigned int ms);
#endif

#endif

