//--------------------------------------------------------------
// Time		:2016-12-14
// Author	:JiaXing Shao
// Effect	:
// Function :
//--------------------------------------------------------------

#ifndef _LOG_H_H
#define _LOG_H_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
using namespace std;
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdarg.h>
#include <time.h>

#ifdef __GNUC__
#include <dirent.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#else
#include <io.h>
#include <Windows.h>
#include <string>
#endif

#ifdef __GNUC__
#define CONFFILE "./Configure.ini"
#else
#define CONFFILE ".\\Configure.ini"
#endif
#define LOG_SIZE (1024 * 10)
#define PATH_LEN 224
#define NAME_LEN 32
#define LogInfo (*g_logs)

void GetConfigureString(string value_name, char *value_buf, int32_t strlen, string default_value, const char *filename);
int ParseMessageHex(char *dst, char *src, int srclen);
string GetSystemTime();

class LogFile
{
public:
	static LogFile *GetInstince();

	template<typename T> LogFile& operator<<(T str)
	{
#ifdef __GNUC__
		pthread_mutex_lock(&m_mutexLock);	
		static char timer[32];
		memset(timer, 0, 32);
		if (!m_osFile.is_open() || !CheckFile())
		{
			OpenFile();
		}
		GetSysTime(timer);
		m_osFile << timer << " :" << str << endl;
		pthread_mutex_unlock(&m_mutexLock);	
#else
		EnterCriticalSection(&m_csLock);
		static char timer[32];
		memset(timer, 0, 32);
		if (!m_osFile || !CheckFile())
		{
			OpenFile();
		}
		GetSysTime(timer);
		m_osFile << timer << " :" << str << endl;
		LeaveCriticalSection(&m_csLock);
#endif
		return (*this);
	};
	LogFile& WriteLog(const char* format, ...);
	LogFile& WriteWarn(const char* format, ...);
	LogFile& WriteErr(const char* format, ...);
	LogFile info()
	{
		return *m_pInstince;
	};
	
private:
	LogFile(void);
	LogFile(const LogFile&);
	virtual ~LogFile();
	LogFile& operator=(const LogFile&);
	
	void Initialize();
	bool OpenFile();
	bool CloseFile();
	bool CheckFile();
	void GetSysTime(char *timer);
private:

#ifdef __GNUC__
	pthread_mutex_t m_mutexLock;
	static pthread_mutex_t m_mutexIns;
#else
	CRITICAL_SECTION m_csLock;
	static HANDLE m_hMutex;
#endif

	static LogFile *m_pInstince;
	time_t m_lasttime;
	char * m_pLogPath;
	char * m_pFileName;
	char *m_pFile;
	ofstream m_osFile;
	
private:
	class Garbo
	{
	public:
		Garbo(){};
		~Garbo()
		{
#ifdef __GNUC__
			//Remove instance
			if (m_pInstince != NULL)
			{
				pthread_mutex_lock(&m_mutexIns);
				if (NULL != m_pInstince)
				{
					delete m_pInstince;
					m_pInstince = NULL;
				}
				pthread_mutex_unlock(&m_mutexIns);
			}
			//Delete cond
			pthread_mutex_destroy(&m_mutexIns);
#else
			//Remove instance
			if (m_pInstince != NULL)
			{
				WaitForSingleObject(m_hMutex, INFINITE);
				if (NULL != m_pInstince)
				{
					delete m_pInstince;
					m_pInstince = NULL;
				}
				ReleaseMutex(m_hMutex);
			}
			//Delete mutex
			if (NULL != m_hMutex)
			{
				CloseHandle(m_hMutex);
				m_hMutex = NULL;
			}
#endif
		};
	};
	static Garbo m_Garbo;
};

extern LogFile *g_logs;

#endif
