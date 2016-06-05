
#include "Socket.h"
#include "ProtocolHTTP.h"
#include <iostream>

namespace Sock = ThorsAnvil::Socket;

int main()
{
    Sock::ServerSocket   server(8080);
    int                  finished    = 0;
    while(!finished)
    {
        Sock::DataSocket  accept  = server.accept();
        Sock::HTTPServer  acceptHTTPServer(accept);

        std::string message;
        acceptHTTPServer.recvMessage(message);
        std::cout << message << "\n";

        acceptHTTPServer.sendMessage("", "OK");
    }
}
