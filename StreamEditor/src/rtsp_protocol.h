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

#include "rtsp_struct.h"

// rtsp client
int rtsp_cmd_options(t_rtsp_info *info, char *buffer);
int rtsp_cmd_describe(t_rtsp_info *info, char *buffer);
int rtsp_cmd_setup(t_rtsp_info *info, char *buffer, int type);
int rtsp_cmd_play(t_rtsp_info *info, char *buffer);
int rtsp_cmd_teardown(t_rtsp_info *info, char *buffer);
bool rtsp_reply_parse(t_rtsp_info *info, char *buffer, int buflen, int type);

// rtsp server
int rtsp_reply_options(t_rtsp_info *info, char *buffer);
int rtsp_reply_describe(t_rtsp_info *info, char *buffer);
int rtsp_reply_setup(t_rtsp_info *info, char *buffer, int type);
int rtsp_reply_play(t_rtsp_info *info, char *buffer);
int rtsp_reply_teardown(t_rtsp_info *info, char *buffer);
bool rtsp_cmd_parse(t_rtsp_info *info, char *buffer, int buflen, int type);

string get_md5_response(t_rtsp_info *info, string type, string url);

string *get_part_string(string str, char c, int &count);
void string_replace(string &str,char c);

#endif

