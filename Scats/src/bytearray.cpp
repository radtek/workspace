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

ByteArray::ByteArray(int size, int count)
{
	if(size <= 0)
		size = 1024 * 1024;
	if(count <= 0)
		count = 1024 * 10;

	pthread_mutex_init(&m_mutexLock, NULL);
	m_totalLen = size;
	m_remainLen = size;
	m_beginPos = 0;
	m_endPos = 0;
	m_buffer = new char[m_totalLen];

	m_nCount = 0;
	m_maxCount = count;
	m_nBeginPos = 0;
	m_nEndPos = 0;
	m_szLen = new int[count];
}

ByteArray::~ByteArray()
{
	pthread_mutex_destroy(&m_mutexLock);
	if(m_buffer != NULL)
	{
		delete [] m_buffer;
		m_buffer = NULL;
	}

	if(m_szLen != NULL)
	{
		delete [] m_szLen;
		m_szLen = NULL;
	}
}

bool ByteArray::put_message(char *buffer, int length)
{
	if(buffer == NULL || length <= 0)
		return false;

	pthread_mutex_lock(&m_mutexLock);
	if(m_remainLen >= length && m_maxCount != m_nCount)
	{
		if(length > (m_totalLen - 1 - m_endPos))
		{
			int len = m_totalLen - 1 - m_endPos;
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

		m_szLen[m_nEndPos] = length;
		if(m_nEndPos == m_maxCount - 1)
			m_nEndPos = 0;
		else
			m_nEndPos += 1;
		m_nCount += 1;
	}
	else
	{
		pthread_mutex_unlock(&m_mutexLock);
		return false;
	}
	pthread_mutex_unlock(&m_mutexLock);
	return true;
}

bool ByteArray::get_message(char *buffer, int &length)
{
	if(buffer == NULL)
		return false;
	if(get_count() <= 0)
		return false;

	pthread_mutex_lock(&m_mutexLock);
	length = m_szLen[m_nBeginPos];
	if(length > m_totalLen - 1 - m_beginPos)
	{
		int len = m_totalLen - 1 - m_beginPos;
		memcpy(buffer, m_buffer + m_beginPos, len);
		memcpy(buffer + len, m_buffer, length - len);
		m_beginPos = length - len;
	}
	else
	{
		memcpy(buffer, m_buffer + m_beginPos, length);
		m_beginPos += length;
		m_remainLen += length;
	}
	if(m_nBeginPos == m_maxCount - 1)
		m_nBeginPos = 0;
	else
		m_nBeginPos += 1;
	m_nCount -= 1;

	pthread_mutex_unlock(&m_mutexLock);
	return true;
}
