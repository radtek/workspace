//--------------------------------------------------------------
// Time		:2016-12-14
// Author	:JiaXing Shao
// Effect	:
// Function :
//--------------------------------------------------------------

#ifndef _MULTITONLOG_H_H
#define _MULTITONLOG_H_H

#include "functions.h"
#include <stdlib.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdarg.h>

#ifdef __GNUC__
#include <dirent.h>
#include <unistd.h>
#include <pthread.h>
#else
#include <io.h>
#include <Windows.h>
#endif

#define LOG_SIZE (1024 * 10)
#define PATH_LEN 224
#define NAME_LEN 32

class MultitonLog
{
public:
	MultitonLog(void);
	virtual ~MultitonLog();

public:
	template<typename T> MultitonLog& operator<<(T str)
	{
#ifdef __GNUC__
		pthread_mutex_lock(&m_mutexLock);	
		if (!m_osFile.is_open() || !CheckFile())
		{
			OpenFile();
		}
		m_osFile << GetSystemTime() << " :" << str << endl;
		pthread_mutex_unlock(&m_mutexLock);	
#else
		EnterCriticalSection(&m_csLock);
		if (!m_osFile || !CheckFile())
		{
			OpenFile();
		}
		m_osFile << GetSystemTime() << " :" << str << endl;
		LeaveCriticalSection(&m_csLock);
#endif
		return (*this);
	};
	MultitonLog& WriteLog(const char* format, ...);
	MultitonLog& WriteWarn(const char* format, ...);
	MultitonLog& WriteErr(const char* format, ...);
	
	void Initialize(string logName);

private:
	bool OpenFile();
	bool CloseFile();
	bool CheckFile();

private:

#ifdef __GNUC__
	pthread_mutex_t m_mutexLock;
#else
	CRITICAL_SECTION m_csLock;
#endif
	time_t m_lasttime;
	char * m_pLogPath;
	char * m_pFileName;
	char *m_pFile;
	ofstream m_osFile;
	char m_buffer[LOG_SIZE];
};

#endif
