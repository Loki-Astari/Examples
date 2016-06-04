
#ifndef THORSANVIL_SOCKET_PROTOCOL_SIMPLE_H
#define THORSANVIL_SOCKET_PROTOCOL_SIMPLE_H

#include "Protocol.h"

namespace ThorsAnvil
{
    namespace Socket
    {

class ProtocolSimple: public Protocol
{
    public:
        using Protocol::Protocol;
        void sendMessage(std::string const& url, std::string const& message) override;
        void recvMessage(std::string& message)                               override;
};

    }
}

#endif

