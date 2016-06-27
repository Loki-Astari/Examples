
TARGET	= Server.Single Server.MultiThread Server.MultiPool8 Server.MultiPool16 Server.Async Server.Async2 Server.NonBlocking


all:	$(TARGET)
clean:
	rm -f *.o $(TARGET)

CC			= $(CXX)
CXXFLAGS	= -std=c++14 -I ../Version2/ -I../Version3/ -I/usr/local/include/
CXXFLAGS	+= -Wall -Wextra -pedantic -Werror
LDFLAGS		+= -L/usr/local/lib/

ProtocolHTTP.o: ../Version3/ProtocolHTTP.cpp
	$(CXX) $(CXXFLAGS) -c -o ProtocolHTTP.o ../Version3/ProtocolHTTP.cpp
Protocol.o:	../Version2/Protocol.cpp
	$(CXX) $(CXXFLAGS) -c -o Protocol.o ../Version2/Protocol.cpp
Socket.o:	../Version2/Socket.cpp
	$(CXX) $(CXXFLAGS) -c -o Socket.o ../Version2/Socket.cpp

Server.MultiPool8.o:  Server.MultiThreadWithQueue.cpp
	$(CXX) $(CXXFLAGS) -DPORT=8082 -DPOOL_MULTI=1 -c -o Server.MultiPool8.o  Server.MultiThreadWithQueue.cpp
Server.MultiPool16.o: Server.MultiThreadWithQueue.cpp
	$(CXX) $(CXXFLAGS) -DPORT=8083 -DPOOL_MULTI=2 -c -o Server.MultiPool16.o Server.MultiThreadWithQueue.cpp

FLAGS_NonBlocking	= -levent -lboost_coroutine -lboost_system

Server.%:  Server.%.o Socket.o Protocol.o ProtocolHTTP.o
	$(CXX) $(CXXFLAGS) -o $@ $^ -lpthread $(FLAGS_$*) $(LDFLAGS)

