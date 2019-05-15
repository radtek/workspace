// =====================================================================================
//	Copyright (C) 2019 by Jiaxing Shao.All rights reserved.
//	文 件 名:  rtp_protocol.cpp
//	作    者:  Jiaxing Shao, 13315567369@163.com
//	版 本 号:  1.0
//	创建时间:  2019年05月08日 00时02分18秒
//	Compiler:  g++
//	描    述:  
// =====================================================================================

#include <list>
using namespace std;
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rtp_protocol.h"

int U(unsigned char *buffer, int &start_bit, int bit_size)
{
	if(bit_size > 32)
	{
		return -1;
	}

	int ret = 0;
	for(int i = 0; i < bit_size; i++)
	{
		ret <<= 1;
		if(buffer[start_bit / 8] & (0x80 >> (start_bit % 8)))
		{
			ret += 1;
		}
		start_bit++;
	}
	return ret;
}

rtp_interleaved_frame_t parse_rtp_frame(unsigned char *buffer)
{
	rtp_interleaved_frame_t frame;
	int start_bit = 0;
	memset(&frame, 0, sizeof(rtp_interleaved_frame_t));
	frame.magic 		= U(buffer, start_bit, 8);
	frame.channel_id 	= U(buffer, start_bit, 8);
	frame.length 		= U(buffer, start_bit, 16);
	return frame;
}

rtp_hdr_t parse_rtp_header(unsigned char *buffer)
{
	rtp_hdr_t header;
	int start_bit = 0;
	memset(&header, 0, sizeof(rtp_hdr_t));

	header.version	= U(buffer, start_bit, 2);
	header.p		= U(buffer, start_bit, 1);
	header.x		= U(buffer, start_bit, 1);
	header.cc		= U(buffer, start_bit, 4);
	header.m		= U(buffer, start_bit, 1);
	header.pt		= U(buffer, start_bit, 7);
	header.seq		= U(buffer, start_bit, 16);
	header.ts		= U(buffer, start_bit, 32);
	header.ssrc		= U(buffer, start_bit, 32);
	for(int i = 0; i < header.cc; i++)
	{
		header.csrc[i] = U(buffer, start_bit, 32);
	}
	return header;
}

rtp_nalu_hdr_t parse_rtp_h264_format(unsigned char *buffer)
{
	rtp_nalu_hdr_t nalu_hdr;
	int start_bit = 0;
	memset(&nalu_hdr, 0, sizeof(rtp_nalu_hdr_t));
	
	nalu_hdr.f		= U(buffer, start_bit, 1);
	nalu_hdr.nri	= U(buffer, start_bit, 2);
	nalu_hdr.type	= U(buffer, start_bit, 5);
	return nalu_hdr;
}

