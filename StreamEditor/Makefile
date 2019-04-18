cc = g++
library_dir = -L /usr/lib/oracle/11.2/client64/lib
include_dir = -I /usr/include/oracle/11.2/client64
libs = -lpthread -levent_core -levent_extra -levent -lociei -lclntsh -lnnz11
objs = main.o logfile.o functions.o rtsp_protocol.o rtsp_client.o md5.o rtsp_server.o http_handle.o http_server.o mongoose.o bytearray.o otlcontrol.o json_value.o \
		json_reader.o json_writer.o rtsp_task.o threadpool.o rtsp_util.o
exe = stream
c_flags = -g -std=c++11


$(exe):$(objs)
	$(cc) $(c_flags) -o $(exe) $(objs) $(libs) $(library_dir)
main.o:
	$(cc) -c $(c_flags) src/main.cpp $(include_dir)
functions.o:src/functions.h
	$(cc) -c $(c_flags) src/functions.cpp
logfile.o:src/logfile.h
	$(cc) -c $(c_flags) src/logfile.cpp
rtsp_client.o:src/rtsp_client.h
	$(cc) -c $(c_flags) src/rtsp_client.cpp $(include_dir)
rtsp_protocol.o:src/rtsp_protocol.h
	$(cc) -c $(c_flags) src/rtsp_protocol.cpp $(include_dir)
rtsp_util.o:src/rtsp_util.h
	$(cc) -c $(c_flags) src/rtsp_util.cpp $(include_dir)
md5.o:src/md5.h
	$(cc) -c $(c_flags) src/md5.cpp
rtsp_server.o:src/rtsp_server.h
	$(cc) -c $(c_flags) src/rtsp_server.cpp $(include_dir)
rtsp_task.o:src/rtsp_task.h
	$(cc) -c $(c_flags) src/rtsp_task.cpp $(include_dir)
http_server.o:src/http_server.h
	$(cc) -c $(c_flags) src/http_server.cpp $(include_dir)
http_handle.o:src/http_handle.h
	$(cc) -c $(c_flags) src/http_handle.cpp $(include_dir)
mongoose.o:src/mongoose/mongoose.h
	$(cc) -c $(c_flags) src/mongoose/mongoose.c $(include_dir)
bytearray.o:src/bytearray.h
	$(cc) -c $(c_flags) src/bytearray.cpp
otlcontrol.o:
	$(cc) -c $(c_flags) src/otlcontrol.cpp $(include_dir)
json_reader.o:src/jsoncpp/json/reader.h
	$(cc) -c $(c_flags) src/jsoncpp/json_reader.cpp
json_writer.o:src/jsoncpp/json/writer.h
	$(cc) -c $(c_flags) src/jsoncpp/json_writer.cpp
json_value.o:src/jsoncpp/json/value.h
	$(cc) -c $(c_flags) src/jsoncpp/json_value.cpp
threadpool.o:src/threadpool.h
	$(cc) -c $(c_flags) src/threadpool.cpp


.PHONY:clean
clean:
	rm -f *.o 
rebuild:
	rm -f *.o
	make

