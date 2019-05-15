// =====================================================================================
//	Copyright (C) 2019 by Jiaxing Shao.All rights reserved.
//	文 件 名:  http_handle.h
//	作    者:  Jiaxing Shao, 13315567369@163.com
//	版 本 号:  1.0
//	创建时间:  2019年04月10日 17时08分26秒
//	Compiler:  g++
//	描    述:  
// =====================================================================================

#ifndef _HTTP_HANDLE_H_H_H
#define _HTTP_HANDLE_H_H_H

#include "http_server.h"

void get_video_stream(void *arg);

bool handle_describe(std::string url, std::string body, mg_connection *c, OnRspCallback rsp_callback);

bool handle_undescribe(std::string url, std::string body, mg_connection *c, OnRspCallback rsp_callback);

#endif
