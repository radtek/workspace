#include "server.h"

sigInfo SigType[] = {{1,"SIGHUP"},{2,"SIGINT"},{3,"SIGQUIT"},{6,"SIGABRT"},{8,"SIGFPE"},{11,"SIGSEGV"},{13,"SIGPIPE"}};

void blackbox(int sig)
{
	printf("Enter blackbox_handler: ");
	printf("SIG name is %s, SIG num is %d\n", strsignal(sig), sig);
		    
	// 打印堆栈信息
	printf("Stack information:\n");
	int j, nptrs;
	#define SIZE 100
	void *buffer[100];
	char **strings;

	nptrs = backtrace(buffer, SIZE);
	printf("backtrace() returned %d addresses\n", nptrs);

	strings = backtrace_symbols(buffer, nptrs);
	if (strings == NULL)
	{
		perror("backtrace_symbol");
		exit(EXIT_FAILURE);
	}
			                                                                         
	for(j = 0; j < nptrs; j++)
		printf("%s\n", strings[j]);
			                                                                                         
	free(strings);
			                                                                                                 
	exit(EXIT_SUCCESS);
}

int main()
{
	//初始化信号处理函数数据结构
	struct sigaction sa;
	memset(&sa,0,sizeof(sa));
	sa.sa_handler = blackbox;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	for(int i = 0;i < sizeof(SigType)/sizeof(sigInfo);i++)
	{
		if(sigaction(SigType[i].num,&sa,NULL) < 0)
		{
			return EXIT_FAILURE;
		}
	}

	iTimeSpace = GetConfigureFromDatabase();
	if(iTimeSpace == -1)
	{
		char space[4] = {0};
		cout<<"获取数据库截止日期配置失败，从本地读取"<<endl;
		GetConfigureString("TIMESPACE",space,4,"30");
		iTimeSpace = atoi(space);
	}
	cout<<"超过此截止天数的数据不再接收："<<iTimeSpace<<" 天"<<endl;

	InitializeOtlInfo();	
	BaseServer serv;
	serv.Start();

	while(1)
	{
		char c = getchar();
		if(c == 'q'||c == 'Q')
			break;
	};

	return 0;
}

