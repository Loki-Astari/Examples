
#include "EventLoop.h"
#include "Common.h"
#include "CommonNonBlocking.h"

int main(int argc, char* argv[])
{
    namespace Sock = ThorsAnvil::Socket;

    std::string         data = Sock::commonSetUp(argc, argv);
    Sock::EventLoop     eventLoop;
    ActionNonBlocking   action(data);
    Sock::EventServer   server(8087, eventLoop, action);

    eventLoop.runLoop();
}

