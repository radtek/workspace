// =====================================================================================
//	Copyright (C) 2019 by Jiaxing Shao.All rights reserved.
//	文 件 名:  rtsp_protocol.cpp
//	作    者:  Jiaxing Shao, 13315567369@163.com
//	版 本 号:  1.0
//	创建时间:  2019年04月02日 11时39分34秒
//	Compiler:  g++
//	描    述:  
// =====================================================================================

#include "functions.h"
#include "md5.h"
#include "logfile.h"
#include "rtsp_protocol.h"

extern LOG_QUEUE *log_queue;

int rtsp_cmd_options(t_rtsp_info *info, char *buffer)
{
	sprintf(buffer, "OPTIONS %s RTSP/1.0\r\n"
					"CSeq: %d\r\n"
					"\r\n", 
					info->rtsp_url, ++info->cmd_seq);
	log_info(log_queue, "Send Command Message: \n%s", buffer);
	return strlen(buffer);
}

int rtsp_cmd_describe(t_rtsp_info *info, char *buffer)
{
	if(info->secret)
	{
		string response = get_md5_response(info, "DESCRIBE", info->rtsp_url);
		sprintf(buffer, "DESCRIBE %s RTSP/1.0\r\n"
						"CSeq: %d\r\n"
						"Authorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\"\r\n"
						"\r\n", 
						info->rtsp_url, ++info->cmd_seq, info->username, info->realm, info->nonce, info->rtsp_url, response.c_str());
	}
	else
	{
		sprintf(buffer, "DESCRIBE %s RTSP/1.0\r\nCSeq: %d\r\n\r\n", info->rtsp_url, ++info->cmd_seq);
	}
	log_info(log_queue, "Send Command Message: \n%s", buffer);
	return strlen(buffer);
}

int rtsp_cmd_setup(t_rtsp_info *info, char *buffer, int type)
{
	if(info->secret)
	{
		string url;
		if(type == 1)
		{
			url = info->video_url;
		}
		else
		{
			url = info->audio_url;
		}
		string response = get_md5_response(info, "SETUP", url);
		sprintf(buffer, "SETUP %s RTSP/1.0\r\n"
							"Transport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n"
							"CSeq: %d\r\n"
							"Authorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\"\\, response=\"%s\"\r\n"
							"\r\n",
							url.c_str(), ++info->cmd_seq, info->username, info->realm,
							info->nonce, url.c_str(), response.c_str());
	}
	else
	{
	}

	log_info(log_queue, "Send Command Message: \n%s", buffer);
	return strlen(buffer);
}

int rtsp_cmd_play(t_rtsp_info *info, char *buffer)
{
	if(info->secret)
	{
		string url = info->rtsp_url;
		string response = get_md5_response(info ,"PLAY", url + "/");
		sprintf(buffer, "PLAY %s/ RTSP/1.0\r\n"
						"Range: npt=0.000-\r\n"
						"CSeq: %d\r\n"
						"Session: %s\r\n"
						"Authorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s/\", response=\"%s\"\r\n"
						"\r\n",
						info->rtsp_url, ++info->cmd_seq, info->session, info->username, 
						info->realm, info->nonce, info->rtsp_url, response.c_str());
	}
	else
	{
	}

	log_info(log_queue, "Send Command Message: \n%s", buffer);
	return strlen(buffer);
}

int rtsp_cmd_teardown(t_rtsp_info *info, char *buffer)
{
	log_info(log_queue, "Send Command Message: \n%s", buffer);
	return strlen(buffer);
}

bool rtsp_reply_parse(t_rtsp_info *info, char *buffer, int buflen, int cmd)
{
	char *buf = new char[buflen - 4 + 1];
	memset(buf, 0, buflen - 4 + 1);
	memcpy(buf, buffer, buflen - 4);
	string message = buf;

	if(buf != NULL)
	{
		delete buf;
		buf = NULL;
	}

	int line_count = 0;
	string *lines = get_part_string(message, "\r\n", line_count);
	for(int i = 0; i < line_count; i++)
	{
		int count = 0;
		string *parts = get_part_string(lines[i], " ", count);
		
		// 根据发送指令解析数据
		switch(cmd)
		{
			case enum_cmd_options:
				break;
			case enum_cmd_describe:
			case enum_cmd_describe_secret:
				if(strcmp(parts[0].c_str(), "RTSP/1.0") == 0)
				{
					if(strcmp(parts[1].c_str(), "401") == 0)
					{
						info->secret = true;
					}
				}
				else if(strcmp(parts[0].c_str(), "WWW-Authenticate:") == 0)
				{
					if(strcmp(parts[1].c_str(), "Digest") == 0)
					{
						for(int j = 2; j < count; j++)
						{
							string_replace(parts[j], '\"');
							string_replace(parts[j], ',');
							int n = 0;
							string *args = get_part_string(parts[j], "=", n);
							do{
								if(n < 2)
								{
									break;
								}
								if(strcmp(args[0].c_str(), "realm") == 0)
								{
									memcpy(info->realm, args[1].c_str(), args[1].length());
								}
								else if(strcmp(args[0].c_str(), "nonce") == 0)
								{
									memcpy(info->nonce, args[1].c_str(), args[1].length());
								}
							} while(0);

							if(args != NULL)
							{
								delete [] args;
								args = NULL;
							}
						}
					}
				}
				else if(strcmp(parts[0].c_str(), "m=video") == 0)
				{
					for(; i < line_count; i++)
					{
						if(strncmp(lines[i].c_str(), "a=control:", 10) == 0)
						{
							string tmp = lines[i].substr(10, lines[i].length());
							memcpy(info->video_url, tmp.c_str(), tmp.length());
						}
						else if(strncmp(lines[i].c_str(), "m=audio", 7) == 0)
						{
							i--;
							break;
						}
					}
				}
				else if(strcmp(parts[0].c_str(), "m=audio") == 0)
				{
					for(; i < line_count; i++)
					{
						if(strncmp(lines[i].c_str(), "a=control:", 10) == 0)
						{
							string tmp = lines[i].substr(10, lines[i].length());
							memcpy(info->audio_url, tmp.c_str(), tmp.length());
						}
						else if(strncmp(lines[i].c_str(), "m=video", 7) == 0)
						{
							i--;
							break;
						}
					}
				}
				break;
			case enum_cmd_setup_video:
			case enum_cmd_setup_audio:
				if(strcmp(parts[0].c_str(), "Session:") == 0)
				{
					int n = 0;
					string *strs = get_part_string(parts[1], ";", n);
					memcpy(info->session, strs[0].c_str(), strs[0].length());
					if(strs != NULL)
					{
						delete [] strs;
						strs = NULL;
					}
				}
				break;
			case enum_cmd_play:
				break;
			case enum_cmd_teardown:
				break;
			default:
				break;
		}
		if(parts != NULL)
		{
			delete [] parts;
			parts = NULL;
		}
	}
	if(lines != NULL)
	{
		delete [] lines;
		lines = NULL;
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

