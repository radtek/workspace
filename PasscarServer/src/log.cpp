#include "log.h"

LogFile g_logs;

void GetConfigureString(string value_name,char * value_buf,int32_t strlen,string default_value)
{
	bool bIsFindAppName = false;    
	bool bIsFindKeyName = false;    
	int position;
	ifstream file(FILENAME,ios::in);

	if(file)
	{
		while(!file.eof())
		{
			string s;
			getline(file,s);

			if(s[0] == '#')
				continue;

			position = s.find("=");
			if(position != s.npos)
			{
				if(strncmp(s.c_str(),value_name.c_str(),position) == 0)
				{
					memcpy(value_buf,s.c_str()+position+1,s.length()-position-1);
					bIsFindKeyName = true;
					break;
				}
			}
		}
	}
	else
	{
		cout<<"Not Found File ["<<FILENAME<<"]"<<endl;
	}

	if(bIsFindKeyName == false)
	{
		cout<<value_name<<" Not Found,use default value:"<<default_value.c_str()<<endl;
		memcpy(value_buf,default_value.c_str(),strlen);
	}
	file.close();
}

void SetConfigureString(string value_name,char * value_buf,int32_t strlen,string default_value)
{
	bool bIsFindAppName = false;    
	bool bIsFindKeyName = false;
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
/****************************************************************************************************************************************************************/

LogFile::LogFile()
{
#if defined _LINUX
	pthread_mutex_init(&lockLog,NULL);
#elif defined _WINDOWS
	InitializeCriticalSection(&lockLog);
#endif
	mLogPath = new char[256];
	mFileName = new char[64];
	memset(mLogPath,0,256);
	memset(mFileName,0,64);
	GetConfigureString("LogPath",mLogPath,256,"./logs");
	FileOpen();
}

LogFile::~LogFile()
{
	FileClose();
#if defined _LINUX
	pthread_mutex_destroy(&lockLog);
#elif defined _WINDOWS
	DeleteCriticalSection(&lockLog);
#endif

	if(mLogPath != NULL)
	{
		delete [] mLogPath;
		mLogPath = NULL;
	}
	if(mFileName != NULL)
	{
		delete [] mFileName;
		mFileName = NULL;
	}
}

bool LogFile::FileOpen()
{
	bool result = true;
	FileClose();

#if defined _LINUX
	if(opendir(mLogPath) == NULL)
		mkdir(mLogPath,S_IRWXU);
#elif defined _WINDOWS
	if(access(mLogPath,0) == -1){
		 if(!CreateDirectory(mLogPath,NULL)){
			 result = false;
		 }
	}
#endif
	memset(mFileName,0,64);
	time_t timer = time(NULL);
	struct tm * local = localtime(&timer);
	sprintf(mFileName,"%0.4d%0.2d%0.2d%0.2d%0.2d%0.2d",local->tm_year+1900,local->tm_mon+1,local->tm_mday,local->tm_hour,local->tm_min,local->tm_sec);

	char * filename = new char[FILENAME_LEN];
	string path = mLogPath,name = mFileName;

	sprintf(filename,"%s//%s.txt",path.c_str(),name.c_str());
	mFile.open(filename,ios::out|ios::app);
	if(mFile)
	{
		result = true;
		m_lasttime = time(NULL);
	}
	else
	{
		result = false;
	}

	if(filename != NULL)
	{
		delete [] filename;
		filename = NULL;
	}
	return result;
}

inline void LogFile::FileClose()
{
	if(mFile)
	{
		mFile.close();
		mFile.clear();
	}
}

void LogFile::WriteLog(const char* format, ...)
{
	if(time(NULL) - m_lasttime > 86400)
	{
#if defined _LINUX
		pthread_mutex_lock(&lockLog);
		FileOpen();
		pthread_mutex_unlock(&lockLog);
#elif defined _WINDOWS
		EnterCriticalSection(&lockLog);
		FileOpen();
		LeaveCriticalSection(&lockLog);
#endif
	}

	if(mFile)
	{
		char buffer[LOG_SIZE] = {0};
		va_list ap;
		va_start(ap, format);
		vsprintf(buffer, format, ap);
		va_end(ap);

#if defined _LINUX
		pthread_mutex_lock(&lockLog);
		mFile << GetSystemTime() << ":" << buffer << endl;
		pthread_mutex_unlock(&lockLog);
#elif defined _WINDOWS
		EnterCriticalSection(&lockLog);
		mFile << GetSystemTime() << ":" << buffer << endl;
		LeaveCriticalSection(&lockLog);
#endif
	}
	else
	{
#if defined _LINUX
		pthread_mutex_lock(&lockLog);
		FileOpen();
		pthread_mutex_unlock(&lockLog);
#elif defined _WINDOWS
		EnterCriticalSection(&lockLog);
		FileOpen();
		LeaveCriticalSection(&lockLog);
#endif
	}
}

