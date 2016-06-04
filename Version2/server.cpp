
#include "Socket.h"
#include "ProtocolSimple.h"
#include <iostream>

namespace Sock = ThorsAnvil::Socket;

int main()
{
    Sock::ServerSocket   server(8080);
    int                  finished    = 0;
    while(!finished)
    {
        Sock::DataSocket  accept  = server.accept();
        Sock::ProtocolSimple acceptSimple(accept);

        std::string message;
        acceptSimple.recvMessage(message);
        std::cout << message << "\n";

        acceptSimple.sendMessage("", "OK");
    }
}
