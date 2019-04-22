#include "functions.h"

void GetConfigureString(string value_name, char * value_buf, unsigned int strlen, string default_value, const char * filename)
{
	bool bIsFindKeyName = false;
	int position = 0;
	int left_tail_pos = 0;
	int right_head_pos = 0;
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

		if(s.length() <= 0)
			continue;

		if (s[0] == '#')
			continue;

		position = s.find("=");
		if (position != s.npos)
		{
			left_tail_pos = position - 1;
			for (int i = left_tail_pos; i > 0; i--)
			{
				if (s[i] == ' ' || s[i] == '\t')
					left_tail_pos -= 1;
				else
					break;
			}

			if (strncmp(s.c_str(), value_name.c_str(), left_tail_pos + 1) == 0)
			{
				right_head_pos = position + 1;
				for (int i = right_head_pos; i < s.length(); i++)
				{
					if (s[i] == ' ' || s[i] == '\t')
						right_head_pos += 1;
					else
						break;
				}

				if(strlen >= s.length() - right_head_pos)
				{
					memcpy(value_buf, s.c_str() + right_head_pos, s.length() - right_head_pos);
					bIsFindKeyName = true;
					break;
				}
				else
				{
					memcpy(value_buf, default_value.c_str(), strlen);
					bIsFindKeyName = true;
					cout << "[" << value_name << "]" << " value is too long, use default value: " << default_value.c_str() << endl;
				}
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

string ParseMessageHex(const unsigned char *src, unsigned int srclen)
{
	if(src == NULL || srclen < 0)
	{
		return "";
	}

	ostringstream oss;
	oss << setfill('0') << uppercase;
	for(int i = 0; i < srclen; i++)
	{
		oss << setw(2) << hex << static_cast<int> (src[i] & 0xFF) << dec  << ' ';
	}
	string str = oss.str().c_str();
	return str;
}

string GetSystemTime()
{
	time_t timer = time(NULL);
	char chTime[32] = { 0 };
	struct tm local;
#ifdef __GNUC__
	localtime_r(&timer, &local);
	sprintf(chTime, "%0.4d-%0.2d-%0.2d %0.2d:%0.2d:%0.2d", local.tm_year + 1900, local.tm_mon + 1, 
			local.tm_mday, local.tm_hour, local.tm_min, local.tm_sec);
#else
	localtime_s(&local, &timer);
	sprintf_s(chTime, 32, "%0.4d-%0.2d-%0.2d %0.2d:%0.2d:%0.2d", local.tm_year + 1900, local.tm_mon + 1, 
			local.tm_mday, local.tm_hour, local.tm_min, local.tm_sec);
#endif
	string strTime = chTime;
	return strTime;
}

#ifdef __GNUC__
int Utf8ToGb2312(char *inbuf, size_t inlen, char *outbuf, size_t outlen)
{
	if(inbuf == NULL || outbuf == NULL)
	{
		return -1;
	}
	iconv_t cd = iconv_open("GB2312", "UTF-8");
	char *pIn = inbuf;
	char *pOut = outbuf;

	// 执行完此函数后，inlen值为转换后剩余长度，outlen为pOut剩余未使用字节长度
	size_t ret = iconv(cd, &pIn, &inlen, &pOut, &outlen);
	iconv_close(cd);
	return inlen;
}

void Sleep(unsigned int ms)
{
	unsigned int sec = ms / 1000;
	unsigned int msec = ms % 1000;
	struct timespec req;
	req.tv_sec = sec;
	req.tv_nsec = msec * 1000 * 1000;
	nanosleep(&req, NULL);
}

#endif
