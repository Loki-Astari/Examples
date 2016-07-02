
#include "Socket.h"
#include "ProtocolHTTP.h"
#include "Common.h"
#include "CommonBlocking.h"

namespace Sock = ThorsAnvil::Socket;

int main(int argc, char* argv[])
{
    std::string         data     = Sock::commonSetUp(argc, argv);
    int                 finished = 0;
    Sock::ServerSocket  server(8080);
    Action              action(8080, data, finished);

    while(!finished)
    {
        Sock::DataSocket  accept  = server.accept();
        worker(std::move(accept), action);
    }
}
