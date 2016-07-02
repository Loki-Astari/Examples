
#include "Socket.h"
#include "ProtocolHTTP.h"
#include "Common.h"
#include "CommonBlocking.h"
#include <thread>

namespace Sock = ThorsAnvil::Socket;

int main(int argc, char* argv[])
{
    std::string         data     = Sock::commonSetUp(argc, argv);
    int                 finished = 0;
    Sock::ServerSocket  server(8081);
    Action              action(8081, data, finished);

    while(!finished)
    {
        Sock::DataSocket  accept  = server.accept();

        std::thread work(Sock::worker<Action>, std::move(accept), std::ref(action));
        work.detach();
    }
}
