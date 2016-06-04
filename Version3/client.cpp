
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

    Sock::ConnectSocket    connect(argv[1], 80);
    std::stringstream      url;
    connect.putMessage(Sock::Post, argv[1], "/message", argv[2]);

    std::string message;
    connect.getMessage(Sock::Post, message);
    std::cout << message << "\n";
}

