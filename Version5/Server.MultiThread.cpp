
#include "Socket.h"
#include "ProtocolHTTP.h"
#include "Common.h"
#include <thread>

namespace Sock = ThorsAnvil::Socket;

int main(int argc, char* argv[])
{
    std::string         data    = Sock::commonSetUp(argc, argv);
    Sock::ServerSocket  server(8081);
    int                 finished    = 0;

    while(!finished)
    {
        Sock::DataSocket  accept  = server.accept();

        std::thread work(Sock::worker, std::move(accept), std::ref(server), std::ref(data), std::ref(finished));
        work.detach();
    }
}
