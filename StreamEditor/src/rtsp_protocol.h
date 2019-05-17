// =====================================================================================
//	Copyright (C) 2019 by Jiaxing Shao.All rights reserved.
//	文 件 名:  rtsp_protocol.h
//	作    者:  Jiaxing Shao, 13315567369@163.com
//	版 本 号:  1.0
//	创建时间:  2019年04月02日 11时34分33秒
//	Compiler:  g++
//	描    述:  
// =====================================================================================

#ifndef __RTSP_PROTOCOL_H_H_H
#define __RTSP_PROTOCOL_H_H_H

using namespace std;
#include <stdio.h>
#include <string.h>

typedef enum 
{
	enum_cmd_options = 0,
	enum_cmd_describe = 1,
	enum_cmd_setup = 2,
	enum_cmd_play = 3,
	enum_cmd_teardown = 4
}enum_rtsp_cmd;

typedef struct
{
	char rtsp_url[128];
	char video_url[128];
	char audio_url[128];
	char transport[64];
	char username[32];
	char password[32];
	char session[64];
	char nonce[64];
	char realm[32];
	char ssrc[2][16];
	int seq[2];
	int rtptime[2];
	char ipaddr[16];
	int cmd_seq;
	bool secret;
	// 当前阶段
	int step;
	// 通道数量
	int chanel;
	// 协议阶段计数
	int counter;
	// 描述信息条数, base, video, audio
	int info_count[3];
	// 描述信息
	char base_info[10][128];
	char video_info[10][128];
	char audio_info[10][128];
}t_rtsp_info;

// rtsp请求组包
int rtsp_cmd_options(t_rtsp_info *info, char *buffer);
int rtsp_cmd_describe(t_rtsp_info *info, char *buffer);
int rtsp_cmd_setup(t_rtsp_info *info, char *buffer);
int rtsp_cmd_play(t_rtsp_info *info, char *buffer);
int rtsp_cmd_teardown(t_rtsp_info *info, char *buffer);

// rtsp应答解析, 返回解析结果
int rtsp_parse_reply_options(t_rtsp_info *info, char *buffer, int buflen);
int rtsp_parse_reply_describe(t_rtsp_info *info, char *buffer, int buflen);
int rtsp_parse_reply_setup(t_rtsp_info *info, char *buffer, int buflen);
int rtsp_parse_reply_play(t_rtsp_info *info, char *buffer, int buflen);
int rtsp_parse_reply_teardown(t_rtsp_info *info, char *buffer, int buflen);

// rtsp应答组包
int rtsp_reply_options(t_rtsp_info *info, char *buffer, int cmd_seq, int &over);
int rtsp_reply_describe(t_rtsp_info *info, char *buffer, int cmd_seq, int &over);
int rtsp_reply_setup(t_rtsp_info *info, char *buffer, int cmd_seq, int &over);
int rtsp_reply_play(t_rtsp_info *info, char *buffer, int cmd_seq, int &over);
int rtsp_reply_teardown(t_rtsp_info *info, char *buffer, int cmd_seq, int &over);

// rtsp请求解析, 返回cmd_seq
int rtsp_parse_cmd_options(t_rtsp_info *info, char *buffer, int buflen);
int rtsp_parse_cmd_describe(t_rtsp_info *info, char *buffer, int buflen);
int rtsp_parse_cmd_setup(t_rtsp_info *info, char *buffer, int buflen);
int rtsp_parse_cmd_play(t_rtsp_info *info, char *buffer, int buflen);
int rtsp_parse_cmd_teardown(t_rtsp_info *info, char *buffer, int buflen);

// 定制解析接口,返回cmd_seq
int rtsp_parse_cmd_options(char *local_url, char *url, char *buffer, int &deviceid);

#endif

