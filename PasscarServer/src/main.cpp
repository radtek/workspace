// test
#include "server.h"

int main(int argc,char* argv[])
{
	InitializeOtlInfo();
	char port[] = "13301";
	Server serv;
	serv.Start(port);
	return 0;
}
