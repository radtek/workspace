cc = g++
include_dir = -I /usr/include/oracle/11.2/client64 
library_dir = -L /usr/local/lib -L /usr/local/lib64 -L /usr/lib/oracle/11.2/client64/lib
libs = -lpthread -lociei -lclntsh -lnnz11
c_tags = -g
obj = main.o tcpclient.o multiton_log.o functions.o singleton_log.o otlcontrol.o utcservice.o threadpool.o tinyxml2.o xmlcodec.o
exe = bin/utcservice

$(exe):$(obj)
	$(cc) $(obj) $(c_tags) -o $(exe) $(include_dir) $(library_dir) $(libs)
multiton_log.o:src/multiton_log.h
	$(cc) $(c_tags) -c src/multiton_log.cpp
functions.o:src/functions.h
	$(cc) $(c_tags) -c src/functions.cpp
tcpclient.o:src/tcpclient.h
	$(cc) $(c_tags) -c src/tcpclient.cpp
otlcontrol.o:src/otlcontrol.h src/otlv4.h
	$(cc) $(c_tags) -c src/otlcontrol.cpp $(include_dir)
singleton_log.o:src/singleton_log.h
	$(cc) $(c_tags) -c src/singleton_log.cpp
utcservice.o:src/utcservice.h
	$(cc) $(c_tags) -c src/utcservice.cpp $(include_dir)
threadpool.o:src/threadpool.h
	$(cc) $(c_tags) -c src/threadpool.cpp
tinyxml2.o:src/tinyxml2.h
	$(cc) $(c_tags) -c src/tinyxml2.cpp
xmlcodec.o:src/xmlcodec.h
	$(cc) $(c_tags) -c src/xmlcodec.cpp
main.o:src/utcservice.h
	$(cc) $(c_tags) -c src/main.cpp $(include_dir)

.PHONY:clean
clean:
	rm -f *.o

