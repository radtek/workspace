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


void byte_array_process_stop(t_rtp_byte_array* &rtp_array)
{
	rtp_array->stop = true;
}


