
#ifndef THORSANVIL_SOCKET_VERSION5_COMMON_H
#define THORSANVIL_SOCKET_VERSION5_COMMON_H

#include "Socket.h"
#include <string>
#include <cstdlib>
#include <signal.h>

namespace ThorsAnvil
{
    namespace Socket
    {

std::string commonSetUp(int argc, char* argv[])
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
    return data;
}

void worker(DataSocket&& accepted, ServerSocket& server, std::string const& data, int& finished)
{
    DataSocket  accept(std::move(accepted));
    HTTPServer  acceptHTTPServer(accept);
    try
    {
        std::string message;
        acceptHTTPServer.recvMessage(message);
        // std::cout << message << "\n";
        if (!finished && message == "Done")
        {
            finished = 1;
            server.stop();
            acceptHTTPServer.sendMessage("", "Stoped");
        }
        else
        {
            acceptHTTPServer.sendMessage("", data);
        }
    }
    catch(DropDisconnectedPipe const& e)
    {
        std::cerr << "Pipe Disconnected: " << e.what() << "\n";
    }
}

    }
}

#endif

