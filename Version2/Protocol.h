
#ifndef THORSANVIL_SOCKET_PROTOCOL_H
#define THORSANVIL_SOCKET_PROTOCOL_H

#include <string>

namespace ThorsAnvil
{
    namespace Socket
    {

class DataSocket;
class Protocol
{
    protected:
        DataSocket&     socket;
    public:
        Protocol(DataSocket& socket);
        ~Protocol();

        virtual void sendMessage(std::string const& url, std::string const& message)    = 0;
        virtual void recvMessage(std::string& message)                                  = 0;
};

    }
}

#endif

