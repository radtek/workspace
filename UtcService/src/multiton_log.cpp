#include "multiton_log.h"

MultitonLog::MultitonLog(void)
{
	m_pLogPath = new char[PATH_LEN];
	m_pFileName = new char[NAME_LEN];
	m_pFile = new char[PATH_LEN + NAME_LEN];
	memset(m_pLogPath, 0, PATH_LEN);
	memset(m_pFileName, 0, NAME_LEN);
	memset(m_pFile, 0, PATH_LEN + NAME_LEN);
}

MultitonLog::~MultitonLog(void)
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

void MultitonLog::Initialize(string logPathName)
{
#ifdef __GNUC__
	pthread_mutex_init(&m_mutexLock, NULL);
	pthread_mutex_lock(&m_mutexLock);
	GetConfigureString("LOG_PATH",m_pLogPath, NAME_LEN, "./logs/", CONFFILE);
	if (opendir(m_pLogPath) == NULL)
	{
		mkdir(m_pLogPath, S_IRWXU);
	}
	memcpy(m_pLogPath + strlen(m_pLogPath), logPathName.c_str(), logPathName.length());
	pthread_mutex_unlock(&m_mutexLock);
#else
	InitializeCriticalSection(&m_csLock);
	EnterCriticalSection(&m_csLock);
	GetConfigureString("LOG_PATH",m_pLogPath, NAME_LEN, ".\\logs\\", CONFFILE);
	if (_access(m_pLogPath, 0) == -1) 
	{
		CreateDirectory(m_pLogPath, NULL);
	}
	memcpy(m_pLogPath + strlen(m_pLogPath), logPathName.c_str(), logPathName.length());
	LeaveCriticalSection(&m_csLock);
#endif
}

MultitonLog& MultitonLog::WriteLog(const char* format, ...)
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

	memset(m_buffer, 0, LOG_SIZE);
	va_list ap;
	va_start(ap, format);

#ifdef __GNUC__
	vsprintf(m_buffer, format, ap);
#else
	vsprintf_s(m_buffer, LOG_SIZE, format, ap);		// windows safe function
#endif
	va_end(ap);

#ifdef __GNUC__
	m_osFile << GetSystemTime() << " :[Info]" << m_buffer << endl;
	pthread_mutex_unlock(&m_mutexLock);
#else
	m_osFile << GetSystemTime() << " :[Info]" << m_buffer << endl;
	LeaveCriticalSection(&m_csLock);
#endif
	return (*this);
}

MultitonLog& MultitonLog::WriteWarn(const char* format, ...)
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

	memset(m_buffer, 0, LOG_SIZE);
	va_list ap;
	va_start(ap, format);

#ifdef __GNUC__
	vsprintf(m_buffer, format, ap);
#else
	vsprintf_s(m_buffer, LOG_SIZE, format, ap);		// windows safe function
#endif
	va_end(ap);

#ifdef __GNUC__
	m_osFile << GetSystemTime() << " :[Warning]" << m_buffer << endl;
	pthread_mutex_unlock(&m_mutexLock);
#else
	m_osFile << GetSystemTime() << " :[Warning]" << m_buffer << endl;
	LeaveCriticalSection(&m_csLock);
#endif
	return (*this);
}

MultitonLog& MultitonLog::WriteErr(const char* format, ...)
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

	memset(m_buffer, 0, LOG_SIZE);
	va_list ap;
	va_start(ap, format);

#ifdef __GNUC__
	vsprintf(m_buffer, format, ap);
#else
	vsprintf_s(m_buffer, LOG_SIZE, format, ap);		// windows safe function
#endif
	va_end(ap);

#ifdef __GNUC__
	m_osFile << GetSystemTime() << " :[Error]" << m_buffer << endl;
	pthread_mutex_unlock(&m_mutexLock);
#else
	m_osFile << GetSystemTime() << " :[Error]" << m_buffer << endl;
	LeaveCriticalSection(&m_csLock);
#endif
	return (*this);
}

bool MultitonLog::CloseFile()
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

bool MultitonLog::OpenFile()
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
	struct tm local;
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

bool MultitonLog::CheckFile()
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


