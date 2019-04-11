// =====================================================================================
//	Copyright (C) 2018 by Jiaxing Shao.All rights reserved.
//	文 件 名:  defines.h
//	作    者:  Jiaxing Shao, 13315567369@163.com
//	版 本 号:  1.0
//	创建时间:  2018年11月24日 16时27分16秒
//	Compiler:  g++
//	描    述:  
// =====================================================================================
//
#include <iostream>
using namespace std;
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdarg.h>

#ifdef __GNUC__
#include <dirent.h>
#include <unistd.h>
#include <pthread.h>
#else
#include <io.h>
#include <winsock2.h>
#include <Windows.h>
#include <process.h>
#pragma comment(lib, "Ws2_32.lib")
#endif


