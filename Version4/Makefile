
all:	client
clean:
	rm -f *.o client

CC			= $(CXX)
CXXFLAGS	= -std=c++14 -I ../Version2/
CXXFLAGS	+= -Wall -Wextra -pedantic -Werror
LDFLAGS		+= -lcurl

client:	client.o
	$(CXX) $(CXXFLAGS) -o client $^ $(LDFLAGS)

