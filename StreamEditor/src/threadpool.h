// =====================================================================================
//	Copyright (C) 2019 by Jiaxing Shao.All rights reserved.
//	文 件 名:  threadpool.h
//	作    者:  Jiaxing Shao, 13315567369@163.com
//	版 本 号:  1.0
//	创建时间:  2019年04月17日 18时13分18秒
//	Compiler:  g++
//	描    述:  
// =====================================================================================

// 线程池任务
typedef struct
{
	// 要执行的函数
	void (*callback)(void *arg);
	// 参数
	void *arg;
	// 参数内存是否需要释放, 默认为true
	bool release;
}t_threadpool_task;

t_threadpool_task *create_threadpool_task();
// 线程池初始化
void threadpool_start(int num, int max_task_num);
// 停止线程池任务
void threadpool_stop();
// 
int threadpool_add_task(t_threadpool_task* &task);
void *threadpool_func(void *arg);
