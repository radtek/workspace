// =====================================================================================
//	Copyright (C) 2019 by Jiaxing Shao.All rights reserved.
//	文 件 名:  rtsp_task.cpp
//	作    者:  Jiaxing Shao, 13315567369@163.com
//	版 本 号:  1.0
//	创建时间:  2019年04月11日 17时53分58秒
//	Compiler:  g++
//	描    述:  
// =====================================================================================

#include <iostream>
#include <map>
using namespace std;
#include <unistd.h>
#include "rtsp_struct.h"

// 当前播放列表,deviceid为key
map<unsigned int, t_video_play_info*> g_mapVideoPlay;
// 互斥锁
pthread_mutex_t g_mapVideoLock = PTHREAD_MUTEX_INITIALIZER;

void video_play_free(t_video_play_info* &player)
{

	if(player->rtsp_serv != NULL)
	{
		for(int i = 0; i < MAX_CLNT_ONLINE; i++)
		{
			if(player->rtsp_serv->clnt[i] != NULL)
			{
				close(player->rtsp_serv->clnt[i]->sockfd);
				delete player->rtsp_serv->clnt[i];
				player->rtsp_serv->clnt[i] = NULL;
			}
		}
		delete player->rtsp_serv;
		player->rtsp_serv = NULL;
	}
	if(player->reply_info != NULL)
	{
		delete player->reply_info;
		player->reply_info = NULL;
	}
	if(player->device_info != NULL)
	{
		delete player->device_info;
		player->device_info = NULL;
	}
	if(player->rtsp_info != NULL)
	{
		delete player->rtsp_info;
		player->rtsp_info = NULL;
	}
	if(player->device_conn != NULL)
	{
		delete player->device_conn;
		player->device_conn = NULL;
	}
	pthread_mutex_destroy(&player->serv_lock);
	if(player != NULL)
	{
		delete player;
		player = NULL;
	}
}

void video_task_add(int deviceid)
{
	pthread_mutex_lock(&g_mapVideoLock);
	t_video_play_info *player = new t_video_play_info;
	player->device_info = NULL;
	player->device_conn = NULL;
	player->rtsp_serv = NULL;
	player->rtsp_info = NULL;
	player->stop = false;
	g_mapVideoPlay[deviceid] = player;
	pthread_mutex_unlock(&g_mapVideoLock);
}

void video_task_remove(int deviceid)
{
	pthread_mutex_lock(&g_mapVideoLock);
	map<unsigned int, t_video_play_info*>::iterator iter;
	iter = g_mapVideoPlay.find(deviceid);
	if(iter != g_mapVideoPlay.end())
	{
		t_video_play_info *player = iter->second;
		g_mapVideoPlay.erase(iter);
	}
	pthread_mutex_unlock(&g_mapVideoLock);
}

t_video_play_info *video_task_get(int deviceid)
{
	t_video_play_info *player = NULL;
	pthread_mutex_lock(&g_mapVideoLock);
	map<unsigned int, t_video_play_info*>::iterator iter;
	iter = g_mapVideoPlay.find(deviceid);
	if(iter != g_mapVideoPlay.end())
	{
		player = iter->second;
	}
	pthread_mutex_unlock(&g_mapVideoLock);
	return player;
}

