
#include "Socket.h"
#include <iostream>

namespace Sock = ThorsAnvil::Socket;

int main()
{
    Sock::ServerSocket   server(8080);
    int                  finished    = 0;
    while(!finished)
    {
        Sock::SocketData  accept  = server.accept();

        std::string message;
        accept.getMessage(message);
        std::cout << message << "\n";

        accept.putMessage("OK");
    }
}
