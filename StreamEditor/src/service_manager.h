// =====================================================================================
//	Copyright (C) 2019 by Jiaxing Shao.All rights reserved.
//	文 件 名:  service_manager.h
//	作    者:  Jiaxing Shao, 13315567369@163.com
//	版 本 号:  1.0
//	创建时间:  2019年04月17日 21时58分09秒
//	Compiler:  g++
//	描    述:  
// =====================================================================================

#ifndef _SERVICE_MANAGER_H_H_H
#define _SERVICE_MANAGER_H_H_H

// 取出完整rtp数据包
int get_rtp_buffer(t_rtp_byte_array* &rtp_array, unsigned char *buf);
// 流处理主线程
void *byte_array_process_start(void *arg);
// 流处理线程停止
void byte_array_process_stop(t_rtp_byte_array* &rtp_array);

#endif
