
#include "Socket.h"
#include "ProtocolHTTP.h"
#include "Common.h"
#include <iostream>

namespace Sock = ThorsAnvil::Socket;

int main(int argc, char* argv[])
{
    std::string data    = Sock::commonSetUp(argc, argv);
    Sock::ServerSocket   server(8080);
    int                  finished    = 0;
    while(!finished)
    {
        Sock::DataSocket  accept  = server.accept();
        Sock::HTTPServer  acceptHTTPServer(accept);
        try
        {
            std::string message;
            acceptHTTPServer.recvMessage(message);
            std::cout << message << "\n";

            acceptHTTPServer.sendMessage("", "OK");
        }
        catch(Sock::DropDisconnectedPipe const& e)
        {
            std::cerr << "Pipe Disconnected: " << e.what() << "\n";
        }
    }
}
