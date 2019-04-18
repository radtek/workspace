// =====================================================================================
//	Copyright (C) 2019 by Jiaxing Shao.All rights reserved.
//	文 件 名:  otlcontrol.h
//	作    者:  Jiaxing Shao, 13315567369@163.com
//	版 本 号:  1.0
//	创建时间:  2019年04月10日 20时02分03秒
//	Compiler:  g++
//	描    述:  
// =====================================================================================

#ifndef _OTL_CONTROL_H_H_H
#define _OTL_CONTROL_H_H_H

#define OTL_ORA11G_R2
#include "otlv4.h"

typedef struct
{
	char conn_str[256];
	pthread_mutex_t lock;
	otl_connect conn;
}t_db_conn;

t_db_conn *create_otl_conn_oracle(const char *user, const char *pwd, const char *dbname, const char *ip, const char *port);
t_db_conn *create_otl_conn(const char *conn_str);
void free_otl_conn(t_db_conn* &conn);
int database_open(t_db_conn *conn);
int database_close(t_db_conn *conn);

#endif

