cc = g++
include_dir = -I /usr/local/include/thrift -I /usr/local/include/boost
library_dir = -L /usr/local/lib -L /usr/local/lib64
libs = -lpthread -lthrift
c_tags = -g
obj = main.o tcpclient.o log.o protocol_scats.o bytearray.o task.o threadpool.o
exe = bin/scats

$(exe):$(obj)
	$(cc) $(obj) $(c_tags) -o $(exe) $(include_dir) $(library_dir) $(libs)
log.o:src/log.h
	$(cc) $(c_tags) -c src/log.cpp
bytearray.o:src/bytearray.h
	$(cc) $(c_tags) -c src/bytearray.cpp
task.o:src/task.h
	$(cc) $(c_tags) -c src/task.cpp
threadpool.o:src/threadpool.h
	$(cc) $(c_tags) -c src/threadpool.cpp
protocol_scats.o:src/protocol_scats.h
	$(cc) $(c_tags) -c src/protocol_scats.cpp
tcpclient.o:src/tcpclient.h
	$(cc) $(c_tags) -c src/tcpclient.cpp
main.o:src/tcpclient.h
	$(cc) $(c_tags) -c src/main.cpp
.PHONY:clean
clean:
	rm -f *.o
