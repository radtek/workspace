// =====================================================================================
//	Copyright (C) 2018 by Jiaxing Shao.All rights reserved.
//	文 件 名:  bytearray.h
//	作    者:  Jiaxing Shao, 13315567369@163.com
//	版 本 号:  1.0
//	创建时间:  2018年06月08日 14时10分10秒
//	Compiler:  g++
//	描    述:  
// =====================================================================================

#include <iostream>
using namespace std;
#include <string.h>
#include <pthread.h>

class ByteArray
{
public:
	ByteArray(int size = 1024*1024, int count = 1024*10);
	~ByteArray();
public:
	bool put_message(char *buffer, int length);
	bool get_message(char *buffer, int &length);
	// 待取数据长度
	int get_length()
	{
		if(get_count() == 0)
			return 0;
		pthread_mutex_lock(&m_mutexLock);
		int n = m_szLen[m_nBeginPos];
		pthread_mutex_unlock(&m_mutexLock);
		return n;
	}

	// 当前数据条数
	int get_count()
	{
		pthread_mutex_lock(&m_mutexLock);
		int n = m_nCount;
		pthread_mutex_unlock(&m_mutexLock);
		return n;
	}

	// 可写数据记录数
	int get_remaincount()
	{
		pthread_mutex_lock(&m_mutexLock);
		int n = m_maxCount - m_nCount;
		pthread_mutex_unlock(&m_mutexLock);
		return n;
	}

	// 可写数据长度
	int get_remainlen()
	{
		pthread_mutex_lock(&m_mutexLock);
		int n = m_remainLen;
		pthread_mutex_unlock(&m_mutexLock);
		return n;
	}
private:
	pthread_mutex_t m_mutexLock;
	char *m_buffer;		//buffer
	int m_remainLen;	//buffer可写长度
	int m_totalLen;		//buffer总长度
	int m_beginPos;		//buffer开始标志位
	int m_endPos;		//buffer结束标志位

	int m_nCount;		//数据记录数
	int m_maxCount;		//buffer存储数据的最大条数
	int *m_szLen;		//每个数据记录的长度
	int m_nBeginPos;	//m_szLen 开始位置
	int m_nEndPos;		//m_szLen 末位位置
};

