
#include "Socket.h"
#include "ProtocolHTTP.h"
#include <iostream>

namespace Sock = ThorsAnvil::Socket;

int main(int argc, char* argv[])
{
    signal(SIGPIPE, SIG_IGN);

    std::string data    = "OK";
    if (argc > 1)
    {
        std::size_t size = std::atoi(argv[1]);
        data.resize(size);
        for(std::size_t loop = 0;loop < size; ++loop)
        {
            data[loop] = 'A' + (loop % 26);
        }
    }
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
            // std::cout << message << "\n";
            acceptHTTPServer.sendMessage("", data);
        }
        catch(Sock::DropDisconnectedPipe const& e)
        {
            std::cerr << "Pipe Disconnected: " << e.what() << "\n";
        }
    }
}
