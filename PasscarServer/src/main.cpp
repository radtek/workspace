// test
#include "server.h"

int main(int argc,char* argv[])
{
	InitializeOtlInfo();
	char port[8] = { 0 };
	GetConfigureString("ServerPort", port, 8, "13301", CONFFILE);
	Server serv;
	serv.Start(port);
	return 0;
}
