// =====================================================================================
//	Copyright (C) 2019 by Jiaxing Shao.All rights reserved.
//	文 件 名:  rtsp_protocol.cpp
//	作    者:  Jiaxing Shao, 13315567369@163.com
//	版 本 号:  1.0
//	创建时间:  2019年04月02日 11时39分34秒
//	Compiler:  g++
//	描    述:  
// =====================================================================================

#include "rtsp_protocol.h"
#include "rtsp_util.h"
#include "logfile.h"

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
						info->rtsp_url, ++info->cmd_seq, info->username, info->realm, 
						info->nonce, info->rtsp_url, response.c_str());
	}
	else
	{
		sprintf(buffer, "DESCRIBE %s RTSP/1.0\r\nCSeq: %d\r\n\r\n", info->rtsp_url, ++info->cmd_seq);
	}
	log_info(log_queue, "Send Command Message: \n%s", buffer);
	return strlen(buffer);
}

int rtsp_cmd_setup(t_rtsp_info *info, char *buffer)
{
	if(info->secret)
	{
		string url;
		string interleaved;
		if(info->counter == 0)
		{
			url = info->video_url;
			interleaved = "0-1";
		}
		else if(info->counter == 1)
		{
			url = info->audio_url;
			interleaved = "2-3";
		}
		string response = get_md5_response(info, "SETUP", url);
		sprintf(buffer, "SETUP %s RTSP/1.0\r\n"
						"Transport: RTP/AVP/TCP;unicast;interleaved=%s\r\n"
						"CSeq: %d\r\n"
						"Authorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\"\r\n"
						"\r\n",
						url.c_str(), interleaved.c_str(), ++info->cmd_seq, info->username, info->realm,
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

/* ***************************************************************************************************************** */
int rtsp_parse_reply_options(t_rtsp_info *info, char *buffer, int buflen)
{
	int ret = 0;
	string message = buffer;
	int nLineCount = 0;
	string *strLines = get_part_string(message, "\r\n", nLineCount);
	do{
		if(strLines == NULL || nLineCount <= 0)
		{
			ret = -1;
			break;
		}

		int nNodeCount = 0;
		string *strNodes = get_part_string(strLines[0], " ", nNodeCount);
		if(strcmp(strNodes[0].c_str(), "RTSP/1.0") == 0 || strcmp(strNodes[1].c_str(), "RTSP/1.1") == 0)
		{
			if(strcmp(strNodes[1].c_str(), "200") != 0)
			{
				ret = -1;
			}
		}
		if(strNodes != NULL)
		{
			delete [] strNodes;
			strNodes = NULL;
		}
	}while(0);

	if(strLines != NULL)
	{
		delete [] strLines;
		strLines = NULL;
	}
	return ret;
}

int rtsp_parse_reply_describe(t_rtsp_info *info, char *buffer, int buflen)
{
	int ret = 0;
	string message = buffer;
	int nLineCount = 0;
	string *strLines = get_part_string(message, "\r\n", nLineCount);
	do{
		if(strLines == NULL || nLineCount <= 0)
		{
			ret = -1;
			break;
		}

		int nNodeCount = 0;
		string *strNodes = get_part_string(strLines[0], " ", nNodeCount);
		if(strcmp(strNodes[0].c_str(), "RTSP/1.0") == 0 || strcmp(strNodes[1].c_str(), "RTSP/1.1") == 0)
		{
			if(strcmp(strNodes[1].c_str(), "200") == 0)
			{
			}
			else if(strcmp(strNodes[1].c_str(), "401") == 0)
			{
				info->secret = true;
				ret = 1;
			}
			else
			{
				ret = -1;
			}
		}
		if(strNodes != NULL)
		{
			delete [] strNodes;
			strNodes = NULL;
		}
		if(ret == -1)
		{
			break;
		}

		for(int i = 1; i < nLineCount; i++)
		{
			nNodeCount = 0;
			strNodes = get_part_string(strLines[i], " ", nNodeCount);
			if(strcmp(strNodes[0].c_str(), "WWW-Authenticate:") == 0)
			{
				if(strcmp(strNodes[1].c_str(), "Digest") == 0)
				{
					for(int j = 2; j < nNodeCount; j++)
					{
						string_replace(strNodes[j], '\"');
						string_replace(strNodes[j], ',');
						int count = 0;
						string *strs = get_part_string(strNodes[j], "=", count);
						if(count >= 2)
						{
							if(strcmp(strs[0].c_str(), "realm") == 0)
							{
								memcpy(info->realm, strs[1].c_str(), strs[1].length());
							}
							else if(strcmp(strs[0].c_str(), "nonce") == 0)
							{
								memcpy(info->nonce, strs[1].c_str(), strs[1].length());
							}
						}
					}
				}
				else if(strcmp(strNodes[0].c_str(), "m=video") == 0)
				{
					info->chanel += 1;
					for(; i < nLineCount; i++)
					{
						if(strncmp(strLines[i].c_str(), "a=control:", 10) == 0)
						{
							string tmp = strLines[i].substr(10, strLines[i].length());
							memcpy(info->video_url, tmp.c_str(), tmp.length());
						}
						else if(strncmp(strLines[i].c_str(), "m=audio", 7) == 0)
						{
							i--;
							break;
						}
					}
				}
				else if(strcmp(strNodes[0].c_str(), "m=audio") == 0)
				{
					info->chanel += 1;
					for(; i < nLineCount; i++)
					{
						if(strncmp(strLines[i].c_str(), "a=control:", 10) == 0)
						{
							string tmp = strLines[i].substr(10, strLines[i].length());
							memcpy(info->audio_url, tmp.c_str(), tmp.length());
						}
						else if(strncmp(strLines[i].c_str(), "m=video", 7) == 0)
						{
							i--;
							break;
						}
					}
				}
			}
			if(strNodes != NULL)
			{
				delete [] strNodes;
				strNodes = NULL;
			}
		}
	}while(0);

	if(strLines != NULL)
	{
		delete [] strLines;
		strLines = NULL;
	}
	return ret;
}

int rtsp_parse_reply_setup(t_rtsp_info *info, char *buffer, int buflen)
{
	int ret = 0;
	string message = buffer;
	int nLineCount = 0;
	string *strLines = get_part_string(message, "\r\n", nLineCount);
	do{
		if(strLines == NULL || nLineCount <= 0)
		{
			ret = -1;
			break;
		}

		int nNodeCount = 0;
		string *strNodes = get_part_string(strLines[0], " ", nNodeCount);
		if(strcmp(strNodes[0].c_str(), "RTSP/1.0") == 0 || strcmp(strNodes[1].c_str(), "RTSP/1.1") == 0)
		{
			if(strcmp(strNodes[1].c_str(), "200") != 0)
			{
				ret = -1;
			}
		}

		if(strNodes != NULL)
		{
			delete [] strNodes;
			strNodes = NULL;
		}

		for(int i = 1; i < nLineCount; i++)
		{
			nNodeCount = 0;
			strNodes = get_part_string(strLines[i], " ", nNodeCount);
			if(strcmp(strNodes[0].c_str(), "Session:") == 0)
			{
				int count = 0;
				string *strs = get_part_string(strNodes[1], ";", count);
				memcpy(info->session, strs[0].c_str(), strs[0].length());
				if(strs != NULL)
				{
					delete [] strs;
					strs = NULL;
				}
			}
			else if(strcmp(strNodes[0].c_str(), "Transport:") == 0)
			{
				int count = 0;
				string *strs = get_part_string(strNodes[1], ";", count);
				for(int j = 2; j < count; j++)
				{
					string_replace(strs[j], '\"');
					string_replace(strs[j], ',');
					int n = 0;
					string *args = get_part_string(strs[j], "=", n);
					if(n >= 2)
					{
						if(strcmp(args[0].c_str(), "ssrc") == 0)
						{
							if(info->counter == 0)
							{
								memcpy(info->ssrc[0], args[1].c_str(), args[1].length());
							}
							else
							{
								memcpy(info->ssrc[1], args[1].c_str(), args[1].length());
							}
						}
					}
					if(args != NULL)
					{
						delete [] args;
						args = NULL;
					}
				}
				if(strs != NULL)
				{
					delete [] strs;
					strs = NULL;
				}
			}
		}	
	}while(0);

	if(strLines != NULL)
	{
		delete [] strLines;
		strLines = NULL;
	}
	return ret;
}

int rtsp_parse_reply_play(t_rtsp_info *info, char *buffer, int buflen)
{
	int ret = 0;
	string message = buffer;
	int nLineCount = 0;
	string *strLines = get_part_string(message, "\r\n", nLineCount);
	do{
		if(strLines == NULL || nLineCount <= 0)
		{
			ret = -1;
			break;
		}

		int nNodeCount = 0;
		string *strNodes = get_part_string(strLines[0], " ", nNodeCount);
		if(strcmp(strNodes[0].c_str(), "RTSP/1.0") == 0 || strcmp(strNodes[1].c_str(), "RTSP/1.1") == 0)
		{
			if(strcmp(strNodes[1].c_str(), "200") != 0)
			{
				ret = -1;
			}
		}
		if(strNodes != NULL)
		{
			delete [] strNodes;
			strNodes = NULL;
		}
	}while(0);

	if(strLines != NULL)
	{
		delete [] strLines;
		strLines = NULL;
	}
	return ret;
}

int rtsp_parse_reply_teardown(t_rtsp_info *info, char *buffer, int buflen)
{
	int ret = 0;
	string message = buffer;
	int nLineCount = 0;
	string *strLines = get_part_string(message, "\r\n", nLineCount);
	do{
		if(strLines == NULL || nLineCount <= 0)
		{
			ret = -1;
			break;
		}

		int nNodeCount = 0;
		string *strNodes = get_part_string(strLines[0], " ", nNodeCount);
		if(strcmp(strNodes[0].c_str(), "RTSP/1.0") == 0 || strcmp(strNodes[1].c_str(), "RTSP/1.1") == 0)
		{
			if(strcmp(strNodes[1].c_str(), "200") != 0)
			{
				ret = -1;
			}
		}
	}while(0);

	if(strLines != NULL)
	{
		delete [] strLines;
		strLines = NULL;
	}
	return ret;
}

/* ***************************************************************************************************************** */
int rtsp_reply_options(t_rtsp_info *info, char *buffer, int cmd_seq)
{
	sprintf(buffer, "RTSP/1.0 200 OK\r\n"
					"CSeq: %d\r\n"
					"Public: OPTIONS, DESCRIBE, PLAY, PAUSE, SETUP, TEARDOWN\r\n"
					"\r\n", cmd_seq);
	return strlen(buffer);
}

int rtsp_reply_describe(t_rtsp_info *info, char *buffer, int cmd_seq)
{
	char context[1024] = { 0 };
	sprintf(context,"v=0\r\n"
					"o=- 1553788149189586 1553788149189586 IN IP4 %s\r\n"
					"s=Media Presentation\r\n"
					"e=NONE\r\n"
					"b=AS:5100\r\n"
					"t=0 0\r\n"
					"a=control:%s/\r\n"
					"m=video 0 RTP/AVP 96\r\n"
					"c=IN IP4 0.0.0.0\r\n"
					"b=AS:5000\r\n"
					"a=recvonly\r\n"
					"a=x-dimensions:2048,1536\r\n"
					"a=control:%s\r\n"
					"a=rtpmap:96 H264/90000\r\n"
					"a=fmtp:96 profile-level-id=420029; packetization-mode=1; sprop-parameter-sets=Z2QAMqwXKgCAAwaEAAAcIAAFfkIQ,aP44sA==\r\n"
					"m=audio 0 RTP/AVP 8\r\n"
					"c=IN IP4 0.0.0.0\r\n"
					"b=AS:50\r\n"
					"a=recvonly\r\n"
					"a=control:%s\r\n"
					"a=rtpmap:8 PCMA/8000\r\n"
					"a=Media_header:MEDIAINFO=494D4B48010100000400000111710110401F000000FA000000000000000000000000000000000000; a=appversion:1.0\r\n"
					"\r\n",
					info->ipaddr, info->rtsp_url, info->video_url, info->audio_url);

	sprintf(buffer, "RTSP/1.0 200 OK\r\n"
			"CSeq: %d\r\n"
			"Content-Type: application/sdp\r\n"
			"Content-Base: %s/\r\n"
			"Content-Length: %d\r\n"
			"\r\n"
			"%s", cmd_seq, info->rtsp_url, strlen(context), context);
			 
	return strlen(buffer);
}

int rtsp_reply_setup(t_rtsp_info *info, char *buffer, int cmd_seq)
{
	string interleaved;
	string ssrc;
	if(info->counter == 0)
	{
		interleaved = "0-1";
		ssrc = info->ssrc[0];
	}
	else
	{
		interleaved = "2-3";
		ssrc = info->ssrc[1];
	}
	sprintf(buffer, "RTSP/1.0 200 OK\r\n"
					"CSeq: %d\r\n"
					"Session:     %s;timeout=60\r\n"
					"Transport: %s;ssrc=%s;mode=\"play\"\r\n"
					"\r\n",
					cmd_seq, info->session, info->transport, interleaved.c_str(), ssrc.c_str());
	return strlen(buffer);
}

int rtsp_reply_play(t_rtsp_info *info, char *buffer, int cmd_seq)
{
	sprintf(buffer, "RTSP/1.0 200 OK\r\n"
					"CSeq: %d\r\n"
					"Session:       %s\r\n"
					"RTP-Info: url=%s/trackID=1;seq=32174;rtptime=296066432,"
							  "url=%s/trackID=2;seq=64348;rtptime=26317072\r\n"
					"\r\n",
					cmd_seq, info->session, info->rtsp_url, info->rtsp_url);
	return strlen(buffer);
}

int rtsp_reply_teardown(t_rtsp_info *info, char *buffer, int cmd_seq)
{
	sprintf(buffer, "RTSP/1.0 200 OK\r\n"
					"CSeq: 1\r\n"
					"Public: OPTIONS, DESCRIBE, PLAY, PAUSE, SETUP, TEARDOWN\r\n"
					"\r\n");
	return strlen(buffer);
}

/* ***************************************************************************************************************** */
int rtsp_parse_cmd_options(t_rtsp_info *info, char *buffer, int buflen)
{
	int ret = 0;
	string message = buffer;
	int nLineCount = 0;
	string *strLines = get_part_string(message, "\r\n", nLineCount);
	do{
		if(strLines == NULL || nLineCount <= 0)
		{
			ret = -1;
			break;
		}

		int nNodeCount = 0;
		string *strNodes = get_part_string(strLines[0], " ", nNodeCount);
		if(strcmp(strNodes[0].c_str(), "OPTIONS") != 0)
		{
			ret = -1;
		}
		if(strNodes != NULL)
		{
			delete [] strNodes;
			strNodes = NULL;
		}
		if(ret == -1)
		{
			break;
		}

		for(int i = 1; i < nLineCount; i++)
		{
			int nNodeCount = 0;
			string *strNodes = get_part_string(strLines[i], " ", nNodeCount);
			if(strcmp(strNodes[0].c_str(), "CSeq:") == 0)
			{
				ret = atoi(strNodes[1].c_str());
			}
			if(strNodes != NULL)
			{
				delete [] strNodes;
				strNodes = NULL;
			}
		}
	}while(0);

	if(strLines != NULL)
	{
		delete [] strLines;
		strLines = NULL;
	}
	return ret;
}

int rtsp_parse_cmd_describe(t_rtsp_info *info, char *buffer, int buflen)
{
	int ret = 0;
	string message = buffer;
	int nLineCount = 0;
	string *strLines = get_part_string(message, "\r\n", nLineCount);
	do{
		if(strLines == NULL || nLineCount <= 0)
		{
			ret = -1;
			break;
		}

		int nNodeCount = 0;
		string *strNodes = get_part_string(strLines[0], " ", nNodeCount);
		if(strcmp(strNodes[0].c_str(), "DESCRIBE") != 0)
		{
			ret = -1;
		}
		if(strNodes != NULL)
		{
			delete [] strNodes;
			strNodes = NULL;
		}
		if(ret == -1)
		{
			break;
		}

		for(int i = 1; i < nLineCount; i++)
		{
			int nNodeCount = 0;
			string *strNodes = get_part_string(strLines[i], " ", nNodeCount);
			if(strcmp(strNodes[0].c_str(), "CSeq:") == 0)
			{
				ret = atoi(strNodes[1].c_str());
			}
			if(strNodes != NULL)
			{
				delete [] strNodes;
				strNodes = NULL;
			}
		}
	}while(0);

	if(strLines != NULL)
	{
		delete [] strLines;
		strLines = NULL;
	}
	return ret;
}

int rtsp_parse_cmd_setup(t_rtsp_info *info, char *buffer, int buflen)
{
	int ret = 0;
	string message = buffer;
	int nLineCount = 0;
	string *strLines = get_part_string(message, "\r\n", nLineCount);
	do{
		if(strLines == NULL || nLineCount <= 0)
		{
			ret = -1;
			break;
		}

		int nNodeCount = 0;
		string *strNodes = get_part_string(strLines[0], " ", nNodeCount);
		if(strcmp(strNodes[0].c_str(), "SETUP") != 0)
		{
			ret = -1;
		}
		if(strNodes != NULL)
		{
			delete [] strNodes;
			strNodes = NULL;
		}
		if(ret == -1)
		{
			break;
		}

		for(int i = 1; i < nLineCount; i++)
		{
			int nNodeCount = 0;
			string *strNodes = get_part_string(strLines[i], " ", nNodeCount);
			if(strcmp(strNodes[0].c_str(), "CSeq:") == 0)
			{
				ret = atoi(strNodes[1].c_str());
			}
			else if(strcmp(strNodes[0].c_str(), "Transport:") == 0)
			{
				memcpy(info->transport, strNodes[1].c_str(), strNodes[1].length());
			}

			if(strNodes != NULL)
			{
				delete [] strNodes;
				strNodes = NULL;
			}
		}
	}while(0);

	if(strLines != NULL)
	{
		delete [] strLines;
		strLines = NULL;
	}
	return ret;
}

int rtsp_parse_cmd_play(t_rtsp_info *info, char *buffer, int buflen)
{
	int ret = 1;
	string message = buffer;
	int nLineCount = 0;
	string *strLines = get_part_string(message, "\r\n", nLineCount);
	do{
		if(strLines == NULL || nLineCount <= 0)
		{
			ret = -1;
			break;
		}

		int nNodeCount = 0;
		string *strNodes = get_part_string(strLines[0], " ", nNodeCount);
		if(strcmp(strNodes[0].c_str(), "PLAY") != 0)
		{
			ret = -1;
		}
		if(strNodes != NULL)
		{
			delete [] strNodes;
			strNodes = NULL;
		}
		if(ret == -1)
		{
			break;
		}

		for(int i = 1; i < nLineCount; i++)
		{
			int nNodeCount = 0;
			string *strNodes = get_part_string(strLines[i], " ", nNodeCount);
			if(strcmp(strNodes[0].c_str(), "CSeq:") == 0)
			{
				ret = atoi(strNodes[1].c_str());
			}
			if(strNodes != NULL)
			{
				delete [] strNodes;
				strNodes = NULL;
			}
		}
	}while(0);

	if(strLines != NULL)
	{
		delete [] strLines;
		strLines = NULL;
	}
	return ret;
}

int rtsp_parse_cmd_teardown(t_rtsp_info *info, char *buffer, int buflen)
{
	int ret = 0;
	string message = buffer;
	int nLineCount = 0;
	string *strLines = get_part_string(message, "\r\n", nLineCount);
	do{
		if(strLines == NULL || nLineCount <= 0)
		{
			ret = -1;
			break;
		}

		int nNodeCount = 0;
		string *strNodes = get_part_string(strLines[0], " ", nNodeCount);
		if(strcmp(strNodes[0].c_str(), "TEARDOWN") != 0)
		{
			ret = -1;
		}
		if(strNodes != NULL)
		{
			delete [] strNodes;
			strNodes = NULL;
		}
		if(ret == -1)
		{
			break;
		}

		for(int i = 1; i < nLineCount; i++)
		{
			int nNodeCount = 0;
			string *strNodes = get_part_string(strLines[i], " ", nNodeCount);
			if(strcmp(strNodes[0].c_str(), "CSeq:") == 0)
			{
				ret = atoi(strNodes[1].c_str());
			}
			if(strNodes != NULL)
			{
				delete [] strNodes;
				strNodes = NULL;
			}
		}
	}while(0);

	if(strLines != NULL)
	{
		delete [] strLines;
		strLines = NULL;
	}
	return ret;
}

/* ***************************************************************************************************************** */
int rtsp_parse_cmd_options(char *buffer, int &deviceid)
{
	int ret = 0;
	string message = buffer;
	int nLineCount = 0;
	string *strLines = get_part_string(message, "\r\n", nLineCount);
	do{
		if(strLines == NULL || nLineCount <= 0)
		{
			ret = -1;
			break;
		}

		int nNodeCount = 0;
		string *strNodes = get_part_string(strLines[0], " ", nNodeCount);
		if(strcmp(strNodes[0].c_str(), "OPTIONS") != 0)
		{
			ret = -1;
		}
		if(strNodes != NULL)
		{
			delete [] strNodes;
			strNodes = NULL;
		}
		if(ret == -1)
		{
			break;
		}

		for(int i = 1; i < nLineCount; i++)
		{
			int nNodeCount = 0;
			string *strNodes = get_part_string(strLines[i], " ", nNodeCount);
			if(strcmp(strNodes[0].c_str(), "CSeq:") == 0)
			{
				ret = atoi(strNodes[1].c_str());
			}
			if(strNodes != NULL)
			{
				delete [] strNodes;
				strNodes = NULL;
			}
		}
	}while(0);

	if(strLines != NULL)
	{
		delete [] strLines;
		strLines = NULL;
	}
	return ret;

}
