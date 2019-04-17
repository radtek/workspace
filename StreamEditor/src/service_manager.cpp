// =====================================================================================
//	Copyright (C) 2019 by Jiaxing Shao.All rights reserved.
//	文 件 名:  service_manager.cpp
//	作    者:  Jiaxing Shao, 13315567369@163.com
//	版 本 号:  1.0
//	创建时间:  2019年04月17日 21时58分05秒
//	Compiler:  g++
//	描    述:  
// =====================================================================================

#include "service_manager.h"

int get_rtp_buffer(t_rtp_byte_array* &rtp_array, unsigned char *buf)
{
	int length = -1;
	if(get_byte_array(rtp_array, (char*)buf, 4))
	{
		length = buf[2] * 256 + buf[3];
		while(true)
		{
			if(get_byte_array(rtp_array, (char*)buf + 4, length))
			{
				break;
			}
			sleep(0.01);
		}
		length += 4;
	}
	return length;
}

void *byte_array_process_start(void *arg)
{
	int deviceid = *((int*)arg);
	log_debug("byte_array_process_start 线程启动, deviceid %d", deviceid);
	pthread_detach(pthread_self());
	t_video_play_info *player = video_task_get(deviceid);
	if(player == NULL)
	{
		log_debug("byte_array_process_start 获取 player 失败, deviceid %d, 线程退出", deviceid);
	}
	unsigned char buffer[255 * 255];
	int length = 0;
	while(true)
	{
		if(player->stop)
		{
			log_debug("byte_array_process_start 线程准备退出, deviceid %d", deviceid);
			break;
		}
		
		length = get_rtp_buffer(player->rtp_array, buffer);
		if(buffer[0] != 0x24)
		{
			byte_array_process_stop(player->rtp_array);
			break;
		}

		if(player->rtsp_serv->clnt_count != 0)
		{
			int clnt_count = player->rtsp_serv->clnt_count;
			for(int i = 0; i < MAX_CLNT_ONLINE; i++)
			{
				if(clnt_count == 0)
				{
					break;
				}
				if(player->rtsp_serv->clnt[i] != NULL)
				{
					send(player->rtsp_serv->clnt[i]->sockfd, buffer, length, 0);
					clnt_count -= 1;
				}
			}
		}
	}
	log_debug("byte_array_process_start 线程退出, deviceid %d", deviceid);
	return NULL;
}

void byte_array_process_stop(t_rtp_byte_array* &rtp_array)
{
	rtp_array->stop = true;
}


