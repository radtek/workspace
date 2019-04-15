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
extern char g_localhost[16];

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
		string interleaved;
		if(type == 1)
		{
			url = info->video_url;
			interleaved = "0-1";
		}
		else
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


int rtsp_reply_options(t_rtsp_reply_info *info, char *buffer)
{
	sprintf(buffer, "RTSP/1.0 200 OK\r\n"
					"CSeq: %d\r\n"
					"Public: OPTIONS, DESCRIBE, PLAY, PAUSE, SETUP, TEARDOWN\r\n"
					"\r\n", info->cmd_seq);
	return strlen(buffer);
}

int rtsp_reply_describe(t_rtsp_reply_info *info, char *buffer)
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
					g_localhost, info->rtsp_url, info->video_url, info->audio_url);

	sprintf(buffer, "RTSP/1.0 200 OK\r\n"
			"CSeq: %d\r\n"
			"Content-Type: application/sdp\r\n"
			"Content-Base: %s/\r\n"
			"Content-Length: %d\r\n"
			"\r\n"
			"%s", info->cmd_seq, info->rtsp_url, strlen(context), context);
			 
	return strlen(buffer);
}

int rtsp_reply_setup(t_rtsp_reply_info *info, char *buffer, int type)
{
	string interleaved;
	string ssrc;
	if(type == 1)
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
					"Session:       %s;timeout=60\r\n"
					"Transport: %s;ssrc=%s;mode=\"play\"\r\n"
					"\r\n",
					info->cmd_seq, info->session, info->transport, interleaved.c_str(), ssrc.c_str());
	return strlen(buffer);
}

int rtsp_reply_play(t_rtsp_reply_info *info, char *buffer)
{
	sprintf(buffer, "RTSP/1.0 200 OK\r\n"
					"CSeq: %d\r\n"
					"Session:       %s\r\n"
					"RTP-Info: url=%s/trackID=1;seq=32174;rtptime=296066432,"
							  "url=%s/trackID=2;seq=64348;rtptime=26317072\r\n"
					"\r\n",
					info->cmd_seq, info->session, info->rtsp_url, info->rtsp_url);
	return strlen(buffer);
}

int rtsp_reply_teardown(t_rtsp_reply_info *info, char *buffer)
{
	sprintf(buffer, "RTSP/1.0 200 OK\r\n"
					"CSeq: 1\r\n"
					"Public: OPTIONS, DESCRIBE, PLAY, PAUSE, SETUP, TEARDOWN\r\n"
					"\r\n");
	return strlen(buffer);
}

/* ***************************************************************************************************************** */
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

