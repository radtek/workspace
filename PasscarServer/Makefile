cc = g++
Library_Dir = /usr/lib/oracle/11.2/client64/lib
Include_Dir = /usr/include/oracle/11.2/client64
Obj = main.o database.o log.o parsemsg.o server.o crc16.o threadpool.o
exe = bin/ccdbs
gdb = -g

$(exe):$(Obj)
		 $(cc) $(Obj) $(gdb) -o $(exe) -lpthread -lociei -lclntsh -lnnz11 -L $(Library_Dir)
main.o:src/main.cpp src/server.h
		 $(cc) -c $(gdb) src/main.cpp -I $(Include_Dir)
crc16.o:src/crc16.cpp src/crc16.h
		 $(cc) -c $(gdb) src/crc16.cpp -I $(Include_Dir)
log.o:src/log.cpp src/log.h
		 $(cc) -c $(gdb) src/log.cpp -I $(Include_Dir)
parsemsg.o:src/parsemsg.cpp src/parsemsg.h src/defines.h
		 $(cc) -c $(gdb) src/parsemsg.cpp -I $(Include_Dir)
database.o:src/database.cpp src/database.h src/otlv4.h
		 $(cc) -c $(gdb) src/database.cpp -I $(Include_Dir)
threadpool.o:src/threadpool.cpp src/threadpool.h
		 $(cc) -c $(gdb) src/threadpool.cpp -I $(Include_Dir)
server.o:src/server.cpp src/server.h
		 $(cc) -c $(gdb) src/server.cpp -I $(Include_Dir)

.PHONY:clean
clean:
		rm -f *.o bin/ccdbs
