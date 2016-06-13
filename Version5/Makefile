
TARGET	= Server.Single Server.MultiThread Server.MultiThreadWithQueue Server.Async Server.NonBlocking

all:	$(TARGET)
clean:
	rm -f *.o $(TARGET)

CC			= $(CXX)
CXXFLAGS	= -std=c++14 -I ../Version2/ -I../Version3/
CXXFLAGS	+= -Wall -Wextra -pedantic -Werror

ProtocolHTTP.o: ../Version3/ProtocolHTTP.cpp
	$(CXX) $(CXXFLAGS) -c -o ProtocolHTTP.o ../Version3/ProtocolHTTP.cpp
Protocol.o:	../Version2/Protocol.cpp
	$(CXX) $(CXXFLAGS) -c -o Protocol.o ../Version2/Protocol.cpp
Socket.o:	../Version2/Socket.cpp
	$(CXX) $(CXXFLAGS) -c -o Socket.o ../Version2/Socket.cpp

Server.%:  Server.%.o Socket.o Protocol.o ProtocolHTTP.o
	$(CXX) $(CXXFLAGS) -o $@ $?

