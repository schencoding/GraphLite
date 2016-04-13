# ----------------------------------------------------------------------
# compiler options
# ----------------------------------------------------------------------

CXX= g++

CFLAGS_COMMON=-std=c++0x -g -O2 -Wall -Wno-strict-aliasing -I../include
#CFLAGS_COMMON=-std=c++0x -g -O0 -DDEBUG_LVL_1 -DDEBUG_LVL_2 -I../include

INCLD_HDFS=-I$(HADOOP_HOME)/include

LIB_HDFS=-L$(HADOOP_HOME)/lib/native -lhdfs
LIB_JAVA= $(shell echo $(JAVA_HOME)/jre/lib/*/server/libjvm.so)

LIB_GRAPHLITE=-lpthread -rdynamic -ldl -lprotobuf-c

#LIB_GRAPHLITE=/usr/lib/x86_64-linux-gnu/libprotobuf-c.a -lpthread -rdynamic -ldl
#LIB_GRAPHLITE=/usr/lib/i386-linux-gnu/libprotobuf-c.a -lpthread -rdynamic -ldl

LIB_GRAPHALGO=-fPIC -shared

# ----------------------------------------------------------------------
# target
# ----------------------------------------------------------------------

all : graphlite
	cp graphlite ../bin

# ----------------------------------------------------------------------
# graphlite
# ----------------------------------------------------------------------

graphlite : WM.begin.pb-c.o MW.begin.pb-c.o WM.curss_finish.pb-c.o MW.nextss_start.pb-c.o MW.end.pb-c.o WM.end.pb-c.o WW.nodemsg_list.pb-c.o Sender.o Receiver.o InputFormatter.o OutputFormatter.o Node.o Worker.o Master.o main.o
	${CXX} ${CFLAGS_COMMON} $^ ${INCLD_HDFS} ${LIB_HDFS} ${LIB_JAVA} ${LIB_GRAPHLITE} -o $@

# ----------------------------------------------------------------------
# protocol buffer c code
# ----------------------------------------------------------------------

%.pb-c.o : %.proto
	protoc-c --c_out=./ $<
	${CXX} -c ${CFLAGS_COMMON} ${@:.o=.c} -o $@

WM.begin.pb-c.o : WM.begin.proto

MW.begin.pb-c.o : MW.begin.proto

WM.curss_finish.pb-c.o : WM.curss_finish.proto

MW.nextss_start.pb-c.o : MW.nextss_start.proto

MW.end.pb-c.o : MW.end.proto

WM.end.pb-c.o : WM.end.proto

WW.nodemsg_list.pb-c.o : WW.nodemsg_list.proto

# ----------------------------------------------------------------------
# C++ source code
# ----------------------------------------------------------------------

%.o : %.cc
	${CXX} -c ${CFLAGS_COMMON} $< ${INCLD_HDFS} -o $@

Master.h : WM.begin.pb-c.o MW.begin.pb-c.o WM.curss_finish.pb-c.o MW.nextss_start.pb-c.o MW.end.pb-c.o WM.end.pb-c.o WW.nodemsg_list.pb-c.o

Worker.h : WM.begin.pb-c.o MW.begin.pb-c.o WM.curss_finish.pb-c.o MW.nextss_start.pb-c.o MW.end.pb-c.o WM.end.pb-c.o WW.nodemsg_list.pb-c.o

InputFormatter.o : InputFormatter.cc ../include/InputFormatter.h Worker.h

OutputFormatter.o : OutputFormatter.cc ../include/OutputFormatter.h Worker.h

Node.o : Node.cc ../include/Node.h Worker.h

Sender.o: Sender.cc Sender.h

Receiver.o: Receiver.cc Receiver.h

Worker.o : Worker.cc Worker.h

Master.o : Master.cc Master.h

main.o : main.cc Master.h Worker.h

# ----------------------------------------------------------------------
# clean up
# ----------------------------------------------------------------------

clean :
	rm -rf graphlite *.o *.so *.pb-c.[ch]
