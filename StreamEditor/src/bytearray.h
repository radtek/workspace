// =====================================================================================
//	Copyright (C) 2018 by Jiaxing Shao.All rights reserved.
//	文 件 名:  bytearray.h
//	作    者:  Jiaxing Shao, 13315567369@163.com
//	版 本 号:  1.0
//	创建时间:  2018年06月08日 14时10分10秒
//	Compiler:  g++
//	描    述:  
// =====================================================================================

#ifndef __BYTEARRAY_H_H_H
#define __BYTEARRAY_H_H_H

#include <iostream>
using namespace std;
#include <string.h>
#ifdef __GNUC__
#include <pthread.h>
#else
// #include<winsock2.h> 必须在 #include<windows.h>之前
#include <winsock2.h>
#include <windows.h>
#endif

class ByteArray
{
public:
	ByteArray(int len = 1024*1024);
	virtual ~ByteArray();
public:
	bool put_message(char *buffer, int length);
	// block: 是否阻塞读取数据, reserve: 读取数据后是否保留
	bool get_message(char *buffer, int length, bool block = false, bool reserve = false);
	void clear_array();

	// 待取数据长度
	int get_length()
	{
#ifdef __GNUC__
		pthread_mutex_lock(&m_mutexLock);
		int n = m_totalLen - m_remainLen;
		pthread_mutex_unlock(&m_mutexLock);
#else
		EnterCriticalSection(&m_csLock);
		int n = m_totalLen - m_remainLen;
		LeaveCriticalSection(&m_csLock);
#endif
		return n;
	}

	// 可写数据长度
	int get_remainlen()
	{
#ifdef __GNUC__
		pthread_mutex_lock(&m_mutexLock);
		int n = m_remainLen;
		pthread_mutex_unlock(&m_mutexLock);
#else
		EnterCriticalSection(&m_csLock);
		int n = m_remainLen;
		LeaveCriticalSection(&m_csLock);
#endif
		return n;
	}
private:
#ifdef __GNUC__
	pthread_mutex_t m_mutexLock;
	pthread_cond_t m_condLock;
#else
	CRITICAL_SECTION m_csLock;
	HANDLE m_hEvent;
#endif

	char *m_buffer;
	int m_remainLen;
	int m_totalLen;
	int m_beginPos;
	int m_endPos;
};

#endif
