
all:	client
clean:
	rm -f *.o client

CC			= $(CXX)
CXXFLAGS	= -std=c++14 -I ../Version2/
CXXFLAGS	+= -Wall -Wextra -pedantic -Werror

Protocol.o:	../Version2/Protocol.cpp
	$(CXX) $(CXXFLAGS) -c -o Protocol.o ../Version2/Protocol.cpp
Socket.o:	../Version2/Socket.cpp
	$(CXX) $(CXXFLAGS) -c -o Socket.o ../Version2/Socket.cpp

client:	client.o Socket.o Protocol.o ProtocolHTTP.o

