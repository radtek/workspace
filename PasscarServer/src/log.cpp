#include "log.h"

LogFile *g_logs = LogFile::GetInstince();;

LogFile *LogFile::m_pInstince = NULL;
LogFile::Garbo LogFile::m_Garbo;
#ifdef __GNUC__
pthread_mutex_t LogFile::m_mutexIns = PTHREAD_MUTEX_INITIALIZER;
#else
HANDLE LogFile::m_hMutex = CreateMutex(NULL, FALSE, NULL);
#endif

void GetConfigureString(string value_name, char * value_buf, int32_t strlen, string default_value, const char * filename)
{
	bool bIsFindKeyName = false;
	int position = 0;
	int temppos = 0;
	ifstream iFile(filename, ios::in);

	if (!iFile)
	{
		cout << "Not Found File [" << filename << "]" << endl;
		ofstream oFile(filename, ios::out | ios::app);
		oFile << value_name << " = " << default_value << endl;
		oFile.close();
		oFile.clear();
		memcpy(value_buf, default_value.c_str(), strlen);
		cout << "[" << value_name << "]" << " Not Found,use default value: " << default_value.c_str() << endl;
		return;
	}

	while (!iFile.eof())
	{
		string s;
		getline(iFile, s);

		if (s[0] == '#')
			continue;

		position = s.find("=");
		if (position != s.npos)
		{
			temppos = position - 1;
			for (int i = temppos; i > 0; i--)
			{
				if (s[i] == ' ' || s[i] == '\n' || s[i] == '\r' || s[i] == '\t')
					temppos -= 1;
				else
					break;
			}
			if (strncmp(s.c_str(), value_name.c_str(), temppos + 1) == 0)
			{
				temppos = position + 1;
				for (int i = temppos; i < s.length(); i++)
				{
					if (s[i] == ' ' || s[i] == '\n' || s[i] == '\r' || s[i] == '\t')
						temppos += 1;
					else
						break;
				}

				memcpy(value_buf, s.c_str() + temppos, s.length() - temppos);
				bIsFindKeyName = true;
				break;
			}
		}
	}
	
	iFile.close();
	iFile.clear();
	
	if (bIsFindKeyName == false)
	{
		ofstream oFile(filename, ios::out | ios::app);
		oFile << value_name << " = " << default_value << endl;
		oFile.close();
		oFile.clear();
		memcpy(value_buf, default_value.c_str(), strlen);
		cout << "[" << value_name << "]" << " Not Found,use default value: " << default_value.c_str() << endl;
	}
}

string GetSystemTime()
{
	char system_time[32] = {0};
	time_t timer = time(NULL);
	struct tm * local = localtime(&timer);
	sprintf(system_time,"%0.4d-%0.2d-%0.2d %0.2d:%0.2d:%0.2d",
		local->tm_year+1900,local->tm_mon+1,local->tm_mday,local->tm_hour,local->tm_min,local->tm_sec);

	return system_time;
}

int ParseMessageHex(char *dst, char *src, int srclen)
{
	if(dst == NULL || src == NULL || srclen < 0)
		return -1;

	ostringstream oss;
	oss << setfill('0') << uppercase;
	for(int i = 0; i < srclen; i++)
	{
		oss << setw(2) << hex << static_cast<int> (src[i] & 0xFF) << dec  << ' ';
	}
	int len = oss.str().length();
	memcpy(dst, oss.str().c_str(), len);
	return len;
}

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

	static char timer[32];
	memset(timer, 0, 32);
	GetSysTime(timer);

#ifdef __GNUC__
	m_osFile << timer << " :[Info]" << buffer << endl;
	pthread_mutex_unlock(&m_mutexLock);
#else
	m_osFile << timer << " :[Info]" << buffer << endl;
	LeaveCriticalSection(&m_csLock);
#endif
	return (*this);
}

LogFile& LogFile::WriteWarn(const char* format, ...)
{
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

	static char timer[32];
	memset(timer, 0, 32);
	GetSysTime(timer);

#ifdef __GNUC__
	m_osFile << timer << " :[Warning]" << buffer << endl;
	pthread_mutex_unlock(&m_mutexLock);
#else
	m_osFile << timer << " :[Warning]" << buffer << endl;
	LeaveCriticalSection(&m_csLock);
#endif
	return (*this);
}

LogFile& LogFile::WriteErr(const char* format, ...)
{
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

	static char timer[32];
	memset(timer, 0, 32);
	GetSysTime(timer);

#ifdef __GNUC__
	m_osFile << timer << " :[Error]" << buffer << endl;
	pthread_mutex_unlock(&m_mutexLock);
#else
	m_osFile << timer << " :[Error]" << buffer << endl;
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
	struct tm * local;
	local = localtime(&timer);
	sprintf(m_pFileName, "%0.4d%0.2d%0.2d", local->tm_year + 1900, local->tm_mon + 1, local->tm_mday);
#else
	struct tm * local = new tm;
	localtime_s(local, &timer);
	sprintf_s(m_pFileName, NAME_LEN, "%0.4d%0.2d%0.2d", local->tm_year + 1900, local->tm_mon + 1, local->tm_mday);
	
	if (local != NULL)
	{
		delete local;
		local = NULL;
	}
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

void LogFile::GetSysTime(char *timer)
{
	time_t temp = time(NULL);
	struct tm *local = new tm;
#ifdef __GNUC__
	local = localtime(&temp);
	sprintf(timer, "%0.4d-%0.2d-%0.2d %0.2d:%0.2d:%0.2d", local->tm_year + 1900, local->tm_mon + 1, 
			local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec);
#else
	localtime_s(local, &temp);
	sprintf_s(timer, 32, "%0.4d-%0.2d-%0.2d %0.2d:%0.2d:%0.2d", local->tm_year + 1900, local->tm_mon + 1, 
			local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec);
#endif
}

