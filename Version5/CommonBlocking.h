#ifndef THORSANVIL_SOCKET_VERSION5_COMMON_BLOCKING_H
#define THORSANVIL_SOCKET_VERSION5_COMMON_BLOCKING_H

#include "Protocol.h"
#include <string>

class ActionBlocking
{
    int                 port;
    std::string const&  data;
    int&                finished;

    public:
        ActionBlocking(int port, std::string const& data, int& finished);
        void stop();
        void log(std::string const& message);

        template<typename Protocol>
        void operator()(std::string const& message, Protocol& protocol)
        {
            // std::cout << message << "\n";
            if (!finished && message == "Done")
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

