// =====================================================================================
//	Copyright (C) 2019 by Jiaxing Shao.All rights reserved.
//	文 件 名:  rtp_protocol.h
//	作    者:  Jiaxing Shao, 13315567369@163.com
//	版 本 号:  1.0
//	创建时间:  2019年05月08日 00时05分16秒
//	Compiler:  g++
//	描    述:  
// =====================================================================================

#ifndef _RTSP_STREAM_H_H_H
#define _RTSP_STREAM_H_H_H

#include "stream_structs.h"

int U(unsigned char *buffer, int &start_bit, int bit_size);

rtp_interleaved_frame_t parse_rtp_frame(unsigned char *buffer);
rtp_hdr_t 				parse_rtp_header(unsigned char *buffer);
rtp_nalu_hdr_t 			parse_rtp_h264_format(unsigned char *buffer);

#endif

