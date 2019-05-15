cc = g++
library_dir = -L /usr/lib/oracle/11.2/client64/lib
include_dir = -I /usr/include/oracle/11.2/client64 -I ./src/include
libs = -lpthread -levent_core -levent_extra -levent -lociei -lclntsh -lnnz11 -lboost_system
objs = main.o logfile.o functions.o rtsp_protocol.o rtsp_client.o md5.o rtsp_server.o http_handle.o http_server.o mongoose.o bytearray.o otlcontrol.o json_value.o \
		json_reader.o json_writer.o rtsp_task.o threadpool.o rtsp_util.o websocket_server.o rtp_protocol.o
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
rtsp_util.o:
	$(cc) -c $(c_flags) src/rtsp_util.cpp $(include_dir)
md5.o:
	$(cc) -c $(c_flags) src/md5.cpp
rtsp_server.o:
	$(cc) -c $(c_flags) src/rtsp_server.cpp $(include_dir)
rtp_protocol.o:
	$(cc) -c $(c_flags) src/coder-decoder/rtp_protocol.cpp $(include_dir)
rtsp_task.o:
	$(cc) -c $(c_flags) src/rtsp_task.cpp $(include_dir)
http_server.o:
	$(cc) -c $(c_flags) src/http_server.cpp $(include_dir)
http_handle.o:
	$(cc) -c $(c_flags) src/http_handle.cpp $(include_dir)
mongoose.o:
	$(cc) -c $(c_flags) src/include/mongoose/mongoose.c $(include_dir)
bytearray.o:
	$(cc) -c $(c_flags) src/bytearray.cpp
otlcontrol.o:
	$(cc) -c $(c_flags) src/otlcontrol.cpp $(include_dir)
json_reader.o:
	$(cc) -c $(c_flags) src/include/jsoncpp/json_reader.cpp
json_writer.o:
	$(cc) -c $(c_flags) src/include/jsoncpp/json_writer.cpp
json_value.o:
	$(cc) -c $(c_flags) src/include/jsoncpp/json_value.cpp
threadpool.o:
	$(cc) -c $(c_flags) src/threadpool.cpp
websocket_server.o:
	$(cc) -c $(c_flags) src/websocket_server.cpp $(include_dir)



.PHONY:clean
clean:
	rm -f *.o 
clean-all:
	rm -f *.o stream
rebuild:
	rm -f *.o & make
