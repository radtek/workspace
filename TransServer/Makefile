cc = g++
Library_Dir = /usr/lib/oracle/11.2/client64/lib
Include_Dir = /usr/include/oracle/11.2/client64
Obj = main.o log.o jdmessage.o database.o pthreadpool.o server.o crc16.o
exe = bin/rabds
gdb = 

$(exe):$(Obj)
		 $(cc) $(Obj) $(gdb) -o $(exe) -lpthread -lociei -lclntsh -lnnz11 -L $(Library_Dir)
main.o:src/main.cpp src/defines.h
		 $(cc) -c $(gdb) src/main.cpp -I $(Include_Dir)
crc16.o:src/crc16.cpp src/crc16.h
		 $(cc) -c $(gdb) src/crc16.cpp 
log.o:src/log.cpp src/log.h src/defines.h
		 $(cc) -c $(gdb) src/log.cpp
jdmessage.o:src/jdmessage.cpp src/jdmessage.h
		 $(cc) -c $(gdb) src/jdmessage.cpp -I $(Include_Dir)
database.o:src/database.cpp src/database.h src/otlv4.h
		 $(cc) -c $(gdb) src/database.cpp -I $(Include_Dir)
pthreadpool.o:src/pthreadpool.cpp src/pthreadpool.h
		 $(cc) -c $(gdb) src/pthreadpool.cpp -I $(Include_Dir)
server.o:src/server.cpp src/server.h
		 $(cc) -c $(gdb) src/server.cpp -I $(Include_Dir)

.PHONY:clean
clean:
		rm -f *.o bin/rabds
