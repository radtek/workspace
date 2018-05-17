#!/bin/bash

CheckProcess()
{
	#检查代入的参数是否有效
	if test -z "$1" ;
	then
		return 1
	fi
	
	#获取目前所运行的进程数目
	PROCESS_NUM=$(ps -ef|grep $1|grep -v "grep"|wc -l)

	#echo "PROCESS_NUM $PROCESS_NUM"
	if [ $PROCESS_NUM -eq 1 ];
	then
		return 0
	else
		return 1
	fi
}

PROCESS_NAME="rabds"

while [ 1 ];do
	CheckProcess $PROCESS_NAME
	CheckQQ_RET=$?

	if [ $CheckQQ_RET -eq 1 ];
	then
		echo '$PROCESS_NUM was not running,restart.'
		$(ps -ef|grep $PROCESS_NAME|grep -v grep|cut -c 9-15|xargs kill -s 9)
		exec ./$PROCESS_NAME &
	fi
	sleep 5
done

