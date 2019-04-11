// =====================================================================================
//	Copyright (C) 2019 by Jiaxing Shao.All rights reserved.
//	文 件 名:  http_server.cpp
//	作    者:  Jiaxing Shao, 13315567369@163.com
//	版 本 号:  1.0
//	创建时间:  2019年04月01日 23时20分25秒
//	Compiler:  g++
//	描    述:  
// =====================================================================================

#include <utility>
#include "http_server.h"

void HttpServer::Init(const std::string &port)
{
	m_port = port;
	s_server_option.enable_directory_listing = "yes";
	s_server_option.document_root = s_web_dir.c_str();
}
	
bool HttpServer::Start()
{
	mg_mgr_init(&m_mgr, NULL);
	mg_connection *connection = mg_bind(&m_mgr, m_port.c_str(), OnHttpEvent);
	if (connection == NULL)
		return false;
	mg_set_protocol_http_websocket(connection);
	printf("starting http server at port: %s\n", m_port.c_str());
	while (true)
		mg_mgr_poll(&m_mgr, 500);
	return true;
}

void HttpServer::OnHttpEvent(mg_connection *connection, int event_type, void *event_data)
{
	http_message *http_req = (http_message *)event_data;
	switch (event_type)
	{
		case MG_EV_HTTP_REQUEST:
			HandleEvent(connection, http_req);
			break;
		default:
			break;
	}
}

static bool route_check(http_message *http_msg, char *route_prefix)
{
	if (mg_vcmp(&http_msg->uri, route_prefix) == 0)
		return true;
	else
		return false;

	mg_vcmp(&http_msg->method, "GET");
	mg_vcmp(&http_msg->method, "POST");
	mg_vcmp(&http_msg->method, "PUT");
	mg_vcmp(&http_msg->method, "DELETE");
}				                                                                                                                                             

void HttpServer::AddHandler(const std::string &url, ReqHandler req_handler)
{
	if (s_handler_map.find(url) != s_handler_map.end())
		return;
	s_handler_map.insert(std::make_pair(url, req_handler));	
}

void HttpServer::RemoveHandler(const std::string &url)
{
	auto it = s_handler_map.find(url);
	if (it != s_handler_map.end())
		s_handler_map.erase(it);
}

void HttpServer::SendRsp(mg_connection *connection, std::string rsp)
{
	mg_printf(connection, "%s", "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n");
	mg_printf_http_chunk(connection, "%s", rsp.c_str());
	mg_send_http_chunk(connection, "", 0);
}

void HttpServer::HandleEvent(mg_connection *connection, http_message *http_req)
{
	std::string req_str = std::string(http_req->message.p, http_req->message.len);
//	printf("got request: %s\n", req_str.c_str());
	std::string url = std::string(http_req->uri.p, http_req->uri.len);
	std::string body = std::string(http_req->body.p, http_req->body.len);
	auto it = s_handler_map.find(url);
	if (it != s_handler_map.end())
	{
		ReqHandler handle_func = it->second;
		handle_func(url, body, connection, SendRsp);	
	}

	if (route_check(http_req, "/"))
	{
		mg_serve_http(connection, http_req, s_server_option);
	}
	else if (route_check(http_req, "/test/hello")) 
	{
		SendRsp(connection, "welcome to httpserver");
	}
	else if (route_check(http_req, "/test/sum"))
	{
		char n1[100], n2[100];
		double result;
		mg_get_http_var(&http_req->body, "n1", n1, sizeof(n1));
		mg_get_http_var(&http_req->body, "n2", n2, sizeof(n2));
		result = strtod(n1, NULL) + strtod(n2, NULL);
		SendRsp(connection, std::to_string(result));
	}
	else
	{
		mg_printf(connection, "%s", "HTTP/1.1 501 Not Implemented\r\nContent-Length: 0\r\n\r\n");
	}
}

bool HttpServer::Close()
{
	mg_mgr_free(&m_mgr);
	return true;
}

