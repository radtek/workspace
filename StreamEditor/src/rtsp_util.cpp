// =====================================================================================
//	Copyright (C) 2019 by Jiaxing Shao.All rights reserved.
//	文 件 名:  rtsp_util.cpp
//	作    者:  Jiaxing Shao, 13315567369@163.com
//	版 本 号:  1.0
//	创建时间:  2019年04月18日 19时55分32秒
//	Compiler:  g++
//	描    述:  
// =====================================================================================

#include "md5.h"
#include "rtsp_util.h"

int send_rtsp_message(int sockfd, char *buffer, int buflen)
{
	for(int i = 0; i < 3; i++)
	{
		int len = send(sockfd, buffer, buflen, 0);
		if(len > 0)
		{
			break;
		}
		else
		{
			if((errno != EINTR && errno != EWOULDBLOCK && errno != EAGAIN))
			{
				return -1;
			}
		}
	}
}

int recv_rtsp_message(int sockfd, char *buffer, int buflen)
{
	int length = 0;
	for(int i = 0; i < 3; i++)
	{
		int len = recv(sockfd, buffer, buflen - length, 0);
		if(len > 0)
		{
			length += len;
			if(buffer[buflen - 2] == '\r' && buffer[buflen - 1] == '\n')
			{
				break;
			}
		}
		else
		{
			if((errno != EINTR && errno != EWOULDBLOCK && errno != EAGAIN))
			{
				return -1;
			}
		}
	}
}

string get_md5_response(t_rtsp_info *info, string cmd, string url)
{
	MD5Encrypt md5;
	char *src_part = (char*)malloc(1024);

	memset(src_part, 0, 1024);
	sprintf(src_part, "%s:%s:%s", info->username, info->realm, info->password);
	string rsp1 = md5.MD5_Encrypt((unsigned char*)src_part, strlen(src_part));

	memset(src_part, 0, 1024);
	sprintf(src_part, "%s:%s", cmd.c_str(), url.c_str());
	string rsp2 = md5.MD5_Encrypt((unsigned char*)src_part, strlen(src_part));

	memset(src_part, 0, 1024);
	sprintf(src_part, "%s:%s:%s", rsp1.c_str(), info->nonce, rsp2.c_str());
	string rsp3 = md5.MD5_Encrypt((unsigned char*)src_part, strlen(src_part));
	free(src_part);

	return rsp3;
}

string *get_part_string(string msg, string separator, int &count)
{
	list<string> list_str;
	int pos = 0;

	while(true)
	{
		int n = msg.find(separator, pos);
		string str = msg.substr(pos, n - pos);
		if(str != "")
		{
			list_str.push_back(str);
			count += 1;
		}
		if(n == -1)
		{
			break;
		}
		pos = n + separator.length();
	}
	string *strs = new string[count];
	list<string>::iterator iter;
	int i = 0;
	for(iter = list_str.begin(); iter != list_str.end(); iter++)
	{
		strs[i++] = (*iter);
	}
	return strs;
}

bool is_separator(char c)
{
	return (c == ' ' || c == ':' || c == ';' || c == ',');
}

void string_replace(string &str,char c)
{
	while(str.find(c) != -1)
	{
		str.replace(str.find(c), 1, "");
	}
}

string get_response_head(string head, string url)
{
	char buffer[256] = { 0 };
	sprintf(buffer, "%s %s RTSP/1.0\r\n");
	string str = buffer;
	return str;
}

string get_response_cesq(int seq)
{
	char buffer[256] = { 0 };
	sprintf(buffer, "CSeq: %d\r\n", seq);
	string str = buffer;
	return str;
}

string get_response_transport()
{
	char buffer[256] = { 0 };
	sprintf(buffer, "Transport: \r\n");
	string str = buffer;
	return str;
}
