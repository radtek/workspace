// =====================================================================================
//	Copyright (C) 2019 by Jiaxing Shao.All rights reserved.
//	文 件 名:  websocket_server.h
//	作    者:  Jiaxing Shao, 13315567369@163.com
//	版 本 号:  1.0
//	创建时间:  2019年05月06日 18时14分46秒
//	Compiler:  g++
//	描    述:  
// =====================================================================================

#ifndef _WEBSOCKET_SERVER_H_H_H
#define _WEBSOCKET_SERVER_H_H_H

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <iostream>
#include <map>
#include <set>
using namespace std;

typedef websocketpp::server<websocketpp::config::asio> server;
typedef server::message_ptr message_ptr;

using websocketpp::connection_hdl;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

using websocketpp::lib::thread;
using websocketpp::lib::mutex;
using websocketpp::lib::lock_guard;
using websocketpp::lib::unique_lock;
using websocketpp::lib::condition_variable;

enum action_type
{
	SUBSCRIBE,
	UNSUBSCRIBE,
	MESSAGE
};

struct action 
{
	action(action_type t, connection_hdl h): type(t), hdl(h) {};
	action(action_type t, connection_hdl h, server::message_ptr m): type(t), hdl(h), msg(m) {};

	action_type type;
	connection_hdl hdl;
	server::message_ptr msg;
};

class WebSocketServer
{
public:
	WebSocketServer();
	~WebSocketServer();
	void start(int port);
	void on_open(connection_hdl hdl);
	void on_close(connection_hdl hdl);
	void on_message(connection_hdl hdl, message_ptr msg);
	static void *process_message(void *arg);
	static void *run(void *arg);
	void send_video_stream(unsigned int deviceid, char *pData, int nSize);
	bool is_subscribe(int deviceid);
private:
	typedef std::set<connection_hdl, std::owner_less<connection_hdl> > conn_list;
	// 订阅列表
	map<unsigned int, conn_list> m_mapSubscribe;
	map<unsigned int, mutex> m_mapLocks;

	server m_server;
	conn_list m_connections;
	mutex m_connection_lock;
};

#endif

