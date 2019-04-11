// =====================================================================================
//	Copyright (C) 2018 by Jiaxing Shao.All rights reserved.
//	文 件 名:  bytearray.cpp
//	作    者:  Jiaxing Shao, 13315567369@163.com
//	版 本 号:  1.0
//	创建时间:  2018年06月08日 14时24分25秒
//	Compiler:  g++
//	描    述:  
// =====================================================================================

#include "bytearray.h"

ByteArray::ByteArray(int len)
{
	if(len <= 0)
	{
		len = 1024 * 1024;
	}

#ifdef __GNUC__
	pthread_mutex_init(&m_mutexLock, NULL);
	pthread_cond_init(&m_condLock, NULL);
#else
	InitializeCriticalSection(&m_csLock);
	m_hEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
#endif
	m_totalLen = len;
	m_remainLen = len;
	m_beginPos = 0;
	m_endPos = 0;
	m_buffer = new char[m_totalLen];
}

ByteArray::~ByteArray()
{
#ifdef __GNUC__
	pthread_mutex_destroy(&m_mutexLock);
	pthread_cond_destroy(&m_condLock);
#else
	DeleteCriticalSection(&m_csLock);
	CloseHandle(m_hEvent);
#endif
	if(m_buffer != NULL)
	{
		delete [] m_buffer;
		m_buffer = NULL;
	}
}

bool ByteArray::put_message(char *buffer, int length)
{
	bool result = false;

	if(buffer == NULL || length <= 0)
	{
		return result;
	}

#ifdef __GNUC__
	pthread_mutex_lock(&m_mutexLock);
#else
	EnterCriticalSection(&m_csLock);
#endif
	if(m_remainLen >= length)
	{
		if(length > (m_totalLen - m_endPos))
		{
			int len = m_totalLen - m_endPos;
			memcpy(m_buffer + m_endPos, buffer, len);
			memcpy(m_buffer, buffer + len, length - len);
			m_endPos = length - len;
			m_remainLen -= length;
		}
		else
		{
			memcpy(m_buffer + m_endPos, buffer, length);
			m_endPos += length;
			m_remainLen -= length;
		}
		result = true;
	}
#ifdef __GNUC__
	pthread_mutex_unlock(&m_mutexLock);
	pthread_cond_signal(&m_condLock);
#else
	LeaveCriticalSection(&m_csLock);
	SetEvent(m_hEvent);
#endif
	return result;
}

bool ByteArray::get_message(char *buffer, int length, bool block, bool reserve)
{
	bool result = false;

#ifdef __GNUC__
	if(buffer == NULL || length <= 0)
	{
		return result;
	}
#else
	if(length <= 0)
	{
		return result;
	}
#endif

#ifdef __GNUC__
	pthread_mutex_lock(&m_mutexLock);
	if(block)
	{
		while(m_totalLen - m_remainLen < length)
		{
			pthread_cond_wait(&m_condLock, &m_mutexLock);
		}
	}
#else
	EnterCriticalSection(&m_csLock);
	if(block)
	{
		while(m_totalLen - m_remainLen < length)
		{
			ResetEvent(m_hEvent);
			LeaveCriticalSection(&m_csLock);
			WaitForSingleObject(m_hEvent, INFINITE);
			EnterCriticalSection(&m_csLock);
		}
	}
#endif
	if(m_totalLen - m_remainLen >= length)
	{
		if(length > m_totalLen - m_beginPos)
		{
			int len = m_totalLen - m_beginPos;
			memcpy(buffer, m_buffer + m_beginPos, len);
			memcpy(buffer + len, m_buffer, length - len);
			if(!reserve)
			{
				m_beginPos = length - len;
				m_remainLen += length;
			}
		}
		else
		{
			memcpy(buffer, m_buffer + m_beginPos, length);
			if(!reserve)
			{
				m_beginPos += length;
				m_remainLen += length;
			}
		}
		result = true;
	}
#ifdef __GNUC__
	pthread_mutex_unlock(&m_mutexLock);
#else
	LeaveCriticalSection(&m_csLock);
#endif
	return result;
}

void ByteArray::clear_array()
{
#ifdef __GNUC__
	pthread_mutex_lock(&m_mutexLock);
	m_remainLen = m_totalLen;
	m_beginPos = 0;
	m_endPos = 0;
	pthread_mutex_unlock(&m_mutexLock);
#else
	EnterCriticalSection(&m_csLock);
	m_remainLen = m_totalLen;
	m_beginPos = 0;
	m_endPos = 0;
	LeaveCriticalSection(&m_csLock);
#endif
}
