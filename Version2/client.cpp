
#include "Socket.h"
#include "ProtocolSimple.h"
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
    Sock::ProtocolSimple   simpleConnect(connect);
    simpleConnect.sendMessage("", argv[2]);

    std::string message;
    simpleConnect.recvMessage(message);
    std::cout << message << "\n";
}

