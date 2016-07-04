#ifndef THORSANVIL_SOCKET_VERSION5_COMMON_H
#define THORSANVIL_SOCKET_VERSION5_COMMON_H

#include "Socket.h"
#include "Utility.h"
#include "ProtocolHTTP.h"
#include <string>
#include <utility>

namespace ThorsAnvil
{
    namespace Socket
    {

std::string commonSetUp(int argc, char* argv[]);

template<typename Action>
inline void worker(DataSocket&& accepted, Action& action)
{
    DataSocket  accept(std::move(accepted));
    HTTPServer  acceptHTTPServer(accept);
    try
    {
        std::string message;
        acceptHTTPServer.recvMessage(message);
        action(message, acceptHTTPServer);
    }
    catch (DropDisconnectedPipe const& e)
    {
        action.log(buildErrorMessage("Pipe Disconnected: ", e.what()));
    }
}

    }
}

#endif
