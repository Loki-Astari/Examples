#ifndef THORSANVIL_SOCKET_VERSION5_COMMON_NONBLOCKING_H
#define THORSANVIL_SOCKET_VERSION5_COMMON_NONBLOCKING_H

#include "Protocol.h"
#include <string>

namespace ThorsAnvil
{
    namespace Socket
    {
        class EventLoop;
    }
}

class ActionNonBlocking
{
    ThorsAnvil::Socket::EventLoop&  loop;
    std::string const&              data;

    public:
        ActionNonBlocking(ThorsAnvil::Socket::EventLoop& loop, std::string const& data);
        void stop();
        void log(std::string const& message);

        template<typename Protocol>
        void operator()(std::string const& message, Protocol& protocol)
        {
            // std::cout << message << "\n";
            if (message == "Done")
            {
                stop();
                protocol.sendMessage("","Stoped");
            }
            else
            {
                protocol.sendMessage("", data);
            }
        }
};
#endif
