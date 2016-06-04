
all:	client server
clean:
	rm -f *.o client server

CC			= $(CXX)
CXXFLAGS	= -std=c++14
CFLAGS		= -Wall -Wextra -pedantic -Werror

client:	client.o Socket.o Protocol.o ProtocolSimple.o
server: server.o Socket.o Protocol.o ProtocolSimple.o

