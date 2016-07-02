#ifndef THORSANVIL_SOCKET_VERSION5_COMMON_NONBLOCKING_H
#define THORSANVIL_SOCKET_VERSION5_COMMON_NONBLOCKING_H

#include "Socket.h"

class Action
{
    std::string const&  data;

    public:
        Action(std::string const& data)
            : data(data)
        {}

        void stop()
        {
        }

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

