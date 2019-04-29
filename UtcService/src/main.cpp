// =====================================================================================
//	Copyright (C) 2018 by Jiaxing Shao.All rights reserved.
//	文 件 名:  main.cpp
//	作    者:  Jiaxing Shao, 13315567369@163.com
//	版 本 号:  1.0
//	创建时间:  2018年08月13日 17时53分24秒
//	Compiler:  g++
//	描    述:  
// =====================================================================================

#include <iostream>
using namespace std;
#include <stdlib.h>
#include "utcservice.h"

int main(int argc, char *argv[])
{
	UtcService utcService;
	utcService.Start();
	return EXIT_SUCCESS;
}

