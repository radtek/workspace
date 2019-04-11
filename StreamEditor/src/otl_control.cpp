// =====================================================================================
//	Copyright (C) 2019 by Jiaxing Shao.All rights reserved.
//	文 件 名:  otl_control.cpp
//	作    者:  Jiaxing Shao, 13315567369@163.com
//	版 本 号:  1.0
//	创建时间:  2019年04月10日 20时02分11秒
//	Compiler:  g++
//	描    述:  
// =====================================================================================

#include "otl_control.h"
static pthread_mutex_t db_lock = PTHREAD_MUTEX_INITIALIZER;
otl_connect db_conn;

string get_otl_conn(char *username, char *password,char *ipaddr, char *port, char *dbname)
{
	char buffer[1024] = { 0 };
	sprintf(buffer, "%s/%s@%s:%s/%s", username, password, ipaddr, port, dbname);
	string conn_str = buffer;
	return conn_str;
}

string get_otl_conn(char *username, char *password, char *dbname)
{
	char buffer[1024] = { 0 };
	sprintf(buffer, "%s/%s@%s", username, password, dbname);
	string conn_str = buffer;
	return conn_str;
}

bool database_open(const char *conn_str)
{
	try
	{
		pthread_mutex_lock(&db_lock);
		otl_connect::otl_initialize();
		db_conn.rlogon(conn_str, 1);
		pthread_mutex_unlock(&db_lock);
		return true;
	}
	catch(otl_exception &p)
	{
		pthread_mutex_unlock(&db_lock);
		return false;
	}
}

bool database_close()
{
	pthread_mutex_lock(&db_lock);
	try
	{
		db_conn.logoff();
	}
	catch(otl_exception &p)
	{
	}
	pthread_mutex_unlock(&db_lock);
}

bool select_device_info(map<unsigned int, t_device_info*> &mapDeviceInfo)
{
	string sql = "select videodeviceid, devicename, devicetype,deviceip,deviceport,reguser,regpwd from T_TVMS_DEVICE";
	try
	{
		pthread_mutex_lock(&db_lock);
		otl_stream select(1, sql.c_str(), db_conn);
		while(!select.eof())
		{
			t_device_info *info = new t_device_info;
			memset(info, 0, sizeof(t_device_info));
			select >> info->deviceid;
			select >> info->crossname;
			select >> info->devicetype;
			select >> info->ipaddr;
			select >> info->port;
			select >> info->username;
			select >> info->password;
			info->rtspport = 554;
			mapDeviceInfo[info->deviceid] = info;
		}
		select.close();
		pthread_mutex_unlock(&db_lock);
	}
	catch(otl_exception &p)
	{
		pthread_mutex_unlock(&db_lock);
		return false;
	}
	return true;
}

