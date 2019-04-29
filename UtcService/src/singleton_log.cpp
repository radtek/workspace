#include "singleton_log.h"

LogFile *g_logs = LogFile::GetInstince();;

LogFile *LogFile::m_pInstince = NULL;
LogFile::Garbo LogFile::m_Garbo;
#ifdef __GNUC__
pthread_mutex_t LogFile::m_mutexIns = PTHREAD_MUTEX_INITIALIZER;
#else
HANDLE LogFile::m_hMutex = CreateMutex(NULL, FALSE, NULL);
#endif

LogFile::LogFile(void)
{

}

LogFile::LogFile(const LogFile&)
{

}

LogFile::~LogFile()
{
#ifdef __GNUC__
	pthread_mutex_lock(&m_mutexLock);
	CloseFile();
	pthread_mutex_unlock(&m_mutexLock);
	pthread_mutex_destroy(&m_mutexLock);
#else
	EnterCriticalSection(&m_csLock);
	CloseFile();
	LeaveCriticalSection(&m_csLock);
	DeleteCriticalSection(&m_csLock);
#endif
	if (m_pLogPath != NULL)
	{
		delete [] m_pLogPath;
		m_pLogPath = NULL;
	}

	if (m_pFileName != NULL)
	{
		delete [] m_pFileName;
		m_pFileName = NULL;
	}

	if (m_pFile != NULL)
	{
		delete [] m_pFile;
		m_pFile = NULL;
	}
}

LogFile& LogFile::operator=(const LogFile&)
{
	return (*this);
}

void LogFile::Initialize()
{
	m_pLogPath = new char[PATH_LEN];
	m_pFileName = new char[NAME_LEN];
	m_pFile = new char[PATH_LEN + NAME_LEN];
	memset(m_pLogPath, 0, PATH_LEN);
	memset(m_pFileName, 0, NAME_LEN);
	memset(m_pFile, 0, PATH_LEN + NAME_LEN);
#ifdef __GNUC__
	pthread_mutex_init(&m_mutexLock, NULL);
	pthread_mutex_lock(&m_mutexLock);
	GetConfigureString("LOG_PATH",m_pLogPath, NAME_LEN, "./logs", CONFFILE);
	pthread_mutex_unlock(&m_mutexLock);
#else
	InitializeCriticalSection(&m_csLock);
	EnterCriticalSection(&m_csLock);
	GetConfigureString("LOG_PATH",m_pLogPath, NAME_LEN, ".\\logs\\", CONFFILE);
	LeaveCriticalSection(&m_csLock);
#endif
}

LogFile *LogFile::GetInstince()
{
	if (m_pInstince == NULL)
	{
#ifdef __GNUC__
		pthread_mutex_lock(&m_mutexIns);
		if (m_pInstince == NULL)
		{
			m_pInstince = new LogFile();
			m_pInstince->Initialize();
		}
		pthread_mutex_unlock(&m_mutexIns);
#else
		WaitForSingleObject(m_hMutex, INFINITE);
		if (m_pInstince == NULL)
		{
			m_pInstince = new LogFile();
			m_pInstince->Initialize();
		}	
		ReleaseMutex(m_hMutex);
#endif
	}
	return m_pInstince;
}

LogFile& LogFile::WriteLog(const char* format, ...)
{
	if(time(NULL) - m_lasttime > 86400) 
	{ 
#ifdef __GNUC__
		pthread_mutex_lock(&m_mutexLock); 
		OpenFile(); 
		pthread_mutex_unlock(&m_mutexLock); 
#else
		EnterCriticalSection(&m_csLock); 
		OpenFile(); 
		LeaveCriticalSection(&m_csLock); 
#endif 
	}

#ifdef __GNUC__
	pthread_mutex_lock(&m_mutexLock);
#else
	EnterCriticalSection(&m_csLock);
#endif
	
	if (!m_osFile || !CheckFile())
	{
		OpenFile();
	}

	static char buffer[LOG_SIZE];
	memset(buffer, 0, LOG_SIZE);
	va_list ap;
	va_start(ap, format);

#ifdef __GNUC__
	vsprintf(buffer, format, ap);
#else
	vsprintf_s(buffer, LOG_SIZE, format, ap);		// windows safe function
#endif
	va_end(ap);

#ifdef __GNUC__
	m_osFile << GetSystemTime() << " :[Info]" << buffer << endl;
	pthread_mutex_unlock(&m_mutexLock);
#else
	m_osFile << GetSystemTime() << " :[Info]" << buffer << endl;
	LeaveCriticalSection(&m_csLock);
#endif
	return (*this);
}

LogFile& LogFile::WriteWarn(const char* format, ...)
{
	if(time(NULL) - m_lasttime > 86400) 
	{ 
#ifdef __GNUC__
		pthread_mutex_lock(&m_mutexLock); 
		OpenFile(); 
		pthread_mutex_unlock(&m_mutexLock); 
#else
		EnterCriticalSection(&m_csLock); 
		OpenFile(); 
		LeaveCriticalSection(&m_csLock); 
#endif 
	}

#ifdef __GNUC__
	pthread_mutex_lock(&m_mutexLock);
#else
	EnterCriticalSection(&m_csLock);
#endif
	
	if (!m_osFile || !CheckFile())
	{
		OpenFile();
	}

	static char buffer[LOG_SIZE];
	memset(buffer, 0, LOG_SIZE);
	va_list ap;
	va_start(ap, format);

#ifdef __GNUC__
	vsprintf(buffer, format, ap);
#else
	vsprintf_s(buffer, LOG_SIZE, format, ap);		// windows safe function
#endif
	va_end(ap);

#ifdef __GNUC__
	m_osFile << GetSystemTime() << " :[Warning]" << buffer << endl;
	pthread_mutex_unlock(&m_mutexLock);
#else
	m_osFile << GetSystemTime() << " :[Warning]" << buffer << endl;
	LeaveCriticalSection(&m_csLock);
#endif
	return (*this);
}

LogFile& LogFile::WriteErr(const char* format, ...)
{
	if(time(NULL) - m_lasttime > 86400) 
	{ 
#ifdef __GNUC__
		pthread_mutex_lock(&m_mutexLock); 
		OpenFile(); 
		pthread_mutex_unlock(&m_mutexLock); 
#else
		EnterCriticalSection(&m_csLock); 
		OpenFile(); 
		LeaveCriticalSection(&m_csLock); 
#endif 
	}

#ifdef __GNUC__
	pthread_mutex_lock(&m_mutexLock);
#else
	EnterCriticalSection(&m_csLock);
#endif
	
	if (!m_osFile || !CheckFile())
	{
		OpenFile();
	}

	static char buffer[LOG_SIZE];
	memset(buffer, 0, LOG_SIZE);
	va_list ap;
	va_start(ap, format);

#ifdef __GNUC__
	vsprintf(buffer, format, ap);
#else
	vsprintf_s(buffer, LOG_SIZE, format, ap);		// windows safe function
#endif
	va_end(ap);

#ifdef __GNUC__
	m_osFile << GetSystemTime() << " :[Error]" << buffer << endl;
	pthread_mutex_unlock(&m_mutexLock);
#else
	m_osFile << GetSystemTime() << " :[Error]" << buffer << endl;
	LeaveCriticalSection(&m_csLock);
#endif
	return (*this);
}

bool LogFile::CloseFile()
{
	try 
	{
		if (m_osFile)
		{
			m_osFile.close();
			m_osFile.clear();
		}
	}
	catch (...)
	{
		return false;
	}
	return true;
}

bool LogFile::OpenFile()
{
	bool result = true;
#ifdef __GNUC__
	if (opendir(m_pLogPath) == NULL)
		mkdir(m_pLogPath, S_IRWXU);
#else	
	if (_access(m_pLogPath, 0) == -1) 
	{
		if (!CreateDirectory(m_pLogPath, NULL)) 
		{
			result = false;
		}
	}
#endif

	memset(m_pFileName, 0, NAME_LEN);
	time_t timer = time(NULL);

#ifdef __GNUC__
	struct tm local;
	localtime_r(&timer, &local);
	sprintf(m_pFileName, "%0.4d%0.2d%0.2d", local.tm_year + 1900, local.tm_mon + 1, local.tm_mday);
#else
	struct tm * local = new tm;
	localtime_s(&local, &timer);
	sprintf_s(m_pFileName, NAME_LEN, "%0.4d%0.2d%0.2d", local.tm_year + 1900, local.tm_mon + 1, local.tm_mday);
#endif
	

	string path = m_pLogPath, name = m_pFileName;
#ifdef __GNUC__
	sprintf(m_pFile, "%s//%s.txt", path.c_str(), name.c_str());
#else
	sprintf_s(m_pFile, PATH_LEN + NAME_LEN, "%s//%s.txt", path.c_str(), name.c_str());
#endif

	CloseFile();
	m_osFile.open(m_pFile, ios::out | ios::app);
	if (m_osFile)
	{
		result = true;
		m_lasttime = time(NULL);
	}
	else
	{
		result = false;
	}

	return result;
}

bool LogFile::CheckFile()
{
#ifdef __GNUC__
	if (opendir(m_pLogPath) == NULL)
		return false;
	else
		return true;
#else
	if (_access(m_pFile, 0) == -1)
		return false;
	else
		return true;
#endif
}

