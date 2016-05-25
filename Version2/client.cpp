
#include "Socket.h"
#include <cstdlib>
#include <iostream>

namespace Sock = ThorsAnvil::Socket;

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        std::cerr << "Usage: client <host> <Message>\n";
        std::exit(1);
    }

    Sock::ConnectSocket    connect(argv[1], 8080);
    connect.putMessage(argv[2]);

    std::string message;
    while(connect.getMessage(message))
    {
        std::cout << message;
    }
    std::cout << "\n";
}

