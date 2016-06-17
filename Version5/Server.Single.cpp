
#include "Socket.h"
#include "ProtocolHTTP.h"
#include "Common.h"

namespace Sock = ThorsAnvil::Socket;

int main(int argc, char* argv[])
{
    std::string data    = Sock::commonSetUp(argc, argv);

    Sock::ServerSocket   server(8080);
    int                  finished    = 0;
    while(!finished)
    {
        Sock::DataSocket  accept  = server.accept();
        worker(std::move(accept), server, data, finished);
    }
}
