// =====================================================================================
//	Copyright (C) 2019 by Jiaxing Shao.All rights reserved.
//	文 件 名:  otlcontrol.cpp
//	作    者:  Jiaxing Shao, 13315567369@163.com
//	版 本 号:  1.0
//	创建时间:  2019年04月10日 20时02分11秒
//	Compiler:  g++
//	描    述:  
// =====================================================================================

#include "otlcontrol.h"
#include <stdio.h>
#include <string.h>
#include <pthread.h>

t_db_conn *create_otl_conn_oracle(const char *user, const char *pwd, const char *dbname, const char *ip, const char *port)
{
	t_db_conn *conn = (t_db_conn*)malloc(sizeof(t_db_conn));
	pthread_mutex_init(&conn->lock, NULL);
	sprintf(conn->conn_str, "%s/%s@%s:%s/%s", user, pwd, ip, port, dbname);
	return conn;
}

t_db_conn *create_otl_conn(const char *conn_str)
{
	if(strlen(conn_str) >= 256)
	{
		return NULL;
	}

	t_db_conn *conn = (t_db_conn*)malloc(sizeof(t_db_conn));
	pthread_mutex_init(&conn->lock, NULL);
	memcpy(conn->conn_str, conn_str, strlen(conn_str));
	return conn;
}

void free_otl_conn(t_db_conn* &conn)
{
	if(conn == NULL)
	{
		return;
	}

	pthread_mutex_destroy(&conn->lock);
	free(conn);
	conn = NULL;
}

int database_open(t_db_conn *conn)
{
	if(conn == NULL)
	{
		return -1;
	}

	try
	{
		pthread_mutex_lock(&conn->lock);
		otl_connect::otl_initialize();
		conn->conn.rlogon(conn->conn_str, 1);
		pthread_mutex_unlock(&conn->lock);
		return 0;
	}
	catch(otl_exception &p)
	{
		pthread_mutex_unlock(&conn->lock);
		return -1;
	}
}

int database_close(t_db_conn *conn)
{
	if(conn == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&conn->lock);
	try
	{
		conn->conn.logoff();
		pthread_mutex_unlock(&conn->lock);
		return 0;
	}
	catch(otl_exception &p)
	{
		pthread_mutex_unlock(&conn->lock);
		return -1;
	}
}


