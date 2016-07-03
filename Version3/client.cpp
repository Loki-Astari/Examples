
#include "Socket.h"
#include "ProtocolHTTP.h"
#include <cstdlib>
#include <iostream>
#include <sstream>

namespace Sock = ThorsAnvil::Socket;

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        std::cerr << "Usage: client <host> <Message>\n";
        std::exit(1);
    }

    Sock::ConnectSocket    connect(argv[1], 8080);
    Sock::HTTPPost         httpConnect(argv[1], connect);
    std::stringstream      url;
    httpConnect.sendMessage("/message", argv[2]);

    std::string message;
    httpConnect.recvMessage(message);
    std::cout << message << "\n";
}

