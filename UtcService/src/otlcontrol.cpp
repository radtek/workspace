/*===================================================================================
 *	
 *	Copyright (C) 2018 by Jiaxing Shao. All rights reserved.
 *		
 * 	文 件 名: otlcontrol.cpp
 * 	作    者: Jiaxing Shao
 * 	创建时间: 2018年05月16日
 *		
====================================================================================*/

#include "otlcontrol.h"

OtlControl::OtlControl()
{
#ifndef __GNUC__
	InitializeCriticalSection(&m_csDatabase);
#else
	pthread_mutex_init(&m_mutexDatabase, NULL);
#endif
	memset(m_strConn, 0, CONNSTRLEN);
}

OtlControl::~OtlControl()
{
	OTLDisConnect();
#ifndef __GNUC__
	DeleteCriticalSection(&m_csDatabase);
#else
	pthread_mutex_destroy(&m_mutexDatabase);
#endif
}

bool OtlControl::OTLConnect()
{
	bool result = true;
#ifndef __GNUC__
	EnterCriticalSection(&m_csDatabase);
#else
	pthread_mutex_lock(&m_mutexDatabase);
#endif
	try{
		otl_connect::otl_initialize(); // initialize OCI environment
		m_dbConn.rlogon(m_strConn,1);
		g_logs->WriteLog("Oracle connect %s succeed!", m_strConn);
	}
	catch(otl_exception &p){
		g_logs->WriteLog("Oracle connect error: %s", p.msg);
		result = false;
	}
#ifndef __GNUC__
	LeaveCriticalSection(&m_csDatabase);
#else
	pthread_mutex_unlock(&m_mutexDatabase);
#endif
	return result;
}

bool OtlControl::OTLDisConnect()
{
#ifndef __GNUC__
	EnterCriticalSection(&m_csDatabase);
#else
	pthread_mutex_lock(&m_mutexDatabase);
#endif
	bool result = true;
	try{
		if(m_dbConn.connected == 1)
		{
			m_dbConn.logoff();
		}
	}
	catch(otl_exception &p){
		g_logs->WriteLog("Oracle close error: %s", p.msg);
		result = false;
	}
#ifndef __GNUC__
	LeaveCriticalSection(&m_csDatabase);
#else
	pthread_mutex_unlock(&m_mutexDatabase);
#endif
	return result;
}

void OtlControl::OTLReConnect()
{
Retry:
	if(m_dbConn.connected == 1)
	{
		try{
			m_dbConn.logoff();
		}
		catch(otl_exception &p){
			g_logs->WriteLog("Oracle close error: %s", p.msg);
		}
	}
	
	try{
		otl_connect::otl_initialize(); // initialize OCI environment
		m_dbConn.rlogon(m_strConn,1);
	}
	catch(otl_exception &p){
		g_logs->WriteLog("Oracle connect failer wait 10s.");
		goto Retry;
	}
	g_logs->WriteLog("Oracle reconnect succeed,[%s].",m_strConn);
}

bool OtlControl::OtlInfoInit()
{
	GetConfigureString("OracleConnStr", m_strConn, CONNSTRLEN, "oracle/123456@127.0.0.1:1521/orcl", CONFFILE);
	if(!OTLConnect())
	{
#ifndef __GNUC__
		Sleep(1000 * 10);
#else
		sleep(10);
#endif
		OTLReConnect();
	}
}

bool OtlControl::GetCcuInfo(vector<T_CcuInfo*> &vecCcu)
{
	bool result = true;
	string select_sql = "select ccuip,lccommport from t_jr_ccu_info";

	try{
		otl_stream _select(1, select_sql.c_str(), m_dbConn);
		while(!_select.eof())
		{
			T_CcuInfo *ccu = new T_CcuInfo;
			memset(ccu, 0, sizeof(T_CcuInfo));
			_select >> ccu->ipaddr;
			_select >> ccu->port;
			ccu->inetAddr = inet_addr(ccu->ipaddr);

			vecCcu.push_back(ccu);
		}
		_select.close();
	}
	catch(otl_exception &p)
	{
		cout << "GetCcuInfo: " << p.msg << endl;
		result = false;
	}
	return result;
}

bool OtlControl::GetCrossInfo(map<unsigned int, T_LcInfo*> &mapCross)
{
	bool result = false;
	string select_sql = "select ctl.crossid,ctl.crossip,ctl.crossport,controlname,ccu.ccuip from t_controller ctl "
		"inner join t_jr_ccu_info ccu on ccu.ccuname = ctl.ccuname where controlname in (9,10) order by ccuip,crossid";

	try{
		otl_stream _select(1, select_sql.c_str(), m_dbConn);
		char ipaddr[IP_SIZE] = { 0 };
		while(!_select.eof())
		{
			memset(ipaddr, 0, IP_SIZE);
			T_LcInfo *lc = new T_LcInfo;
			memset(lc, 0, sizeof(T_LcInfo));
			_select >> lc->crossId;
			_select >> lc->ipaddr;
			_select >> lc->port;
			_select >> lc->type;
			_select >> ipaddr;

			lc->ccuip_inetAddr = inet_addr(ipaddr);
			mapCross[lc->crossId] = lc;
		}
		_select.close();
		result = true;
	}
	catch(otl_exception &p)
	{
		cout << "GetCrossInfo: " << p.msg << endl;
	}
	return result;
}

