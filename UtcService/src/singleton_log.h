//--------------------------------------------------------------
// Time		:2016-12-14
// Author	:JiaXing Shao
// Effect	:
// Function :
//--------------------------------------------------------------

#ifndef _SINGLETON_LOG_H_H_H
#define _SINGLETON_LOG_H_H_H

#include "defines.h"
#include "functions.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <unistd.h>
#include <dirent.h>

#define LOG_SIZE (1024 * 10)
#define PATH_LEN 224
#define NAME_LEN 32
#define LogInfo (*g_logs)

#define WriteLog(format, ...) Logging(emLogTypeInfo, format, ##__VA_ARGS__)
#define WriteWarn(format, ...) Logging(emLogTypeWarn, format, ##__VA_ARGS__)
#define WriteErr(format, ...) Logging(emLogTypeError, format, ##__VA_ARGS__)

enum EnumLogType{
	emLogTypeInfo = 1,
	emLogTypeWarn = 2,
	emLogTypeError = 3
};

class LogFile
{
public:
	static LogFile *GetInstince();

	template<typename T> LogFile& operator<<(T str)
	{
#ifdef __GNUC__
		pthread_mutex_lock(&m_mutexLock);	
		if (!m_osFile.is_open())
		{
			OpenFile();
		}
		m_osFile << GetSystemTime() << ": " << str << endl;
		pthread_mutex_unlock(&m_mutexLock);	
#else
		EnterCriticalSection(&m_csLock);
		if (!m_osFile || !CheckFile())
		{
			OpenFile();
		}
		m_osFile << GetSystemTime() << ": " << str << endl;
		LeaveCriticalSection(&m_csLock);
#endif
		return (*this);
	};
	LogFile& Logging(EnumLogType type,const char* format, ...);
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
