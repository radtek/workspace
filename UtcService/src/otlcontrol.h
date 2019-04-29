/*===================================================================================
 *	
 *	Copyright (C) 2018 by Jiaxing Shao. All rights reserved.
 *		
 * 	文 件 名: otlcontrol.h
 * 	作    者: Jiaxing Shao
 * 	创建时间: 2018年05月16日
 *		
====================================================================================*/

#ifndef _OTLCONTROL_H_H_H
#define _OTLCONTROL_H_H_H

#define OTL_ORA11G_R2
#include "otlv4.h"

#include "singleton_log.h"
#include "defines.h"
#include <vector>
#include <map>
using namespace std;
#include <arpa/inet.h>

#define CONNSTRLEN	128

class OtlControl
{
public:
	OtlControl();
	virtual ~OtlControl();
	bool OtlInfoInit();
	bool GetCcuInfo(vector<T_CcuInfo*> &vecCcu);
	bool GetCrossInfo(map<unsigned int, T_LcInfo*> &mapCross);
private:
	bool OTLConnect();
	bool OTLDisConnect();
	void OTLReConnect();
#ifdef __GNUC__
	pthread_mutex_t m_mutexDatabase;
#else
	CRITICAL_SECTION m_csDatabase;
#endif
	otl_connect m_dbConn;
	char m_strConn[CONNSTRLEN];
};

#endif

