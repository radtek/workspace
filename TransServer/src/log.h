//--------------------------------------------------------------
// Time		:2016-12-14
// Author	:邵佳兴
// Effect	:创建server服务端，监听client的连接
//--------------------------------------------------------------

#include <iostream>
#include <fstream>
using namespace std;
#include <stdio.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdarg.h>
#include <time.h>

#define _LINUX

#ifndef _LOG_H_H
#define _LOG_H_H

#if defined _LINUX
	#include <dirent.h>
	#include <unistd.h>
	#include <pthread.h>
	#include <string.h>
#elif defined _WINDOWS
	#include <io.h>
	#include <Windows.h>
	#include <string>
#endif

#define FILENAME "./Configure.ini"
#define LOG_SIZE (1024 * 10)
#define FILENAME_LEN 256

void GetConfigureString(string value_name,char * value_buf,int32_t strlen,string default_value);
void SetConfigureString(string value_name,char * value_buf,int32_t strlen,string default_value);
string GetSystemTime(void);

class LogFile
{
public:
	LogFile();
	virtual ~LogFile();
/*
	friend ostream &operator <<(string str)
	{
		return WriteLog(str);
	}
*/
	void WriteLog(const char* format,...);
protected:

#if defined _LINUX
	pthread_mutex_t lockLog;
#elif defined _WINDOWS
	CRITICAL_SECTION lockLog;
#endif

	time_t m_lasttime;
	char * mLogPath;
	char * mFileName;
	ofstream mFile;
protected:
	bool FileOpen();
	void FileClose();
};

extern LogFile g_logs;

#endif

