#ifndef THORSANVIL_SOCKET_VERSION5_COMMON_BLOCKING_H
#define THORSANVIL_SOCKET_VERSION5_COMMON_BLOCKING_H

#include "Socket.h"

class ActionBlocking
{
    int                 port;
    std::string const&  data;
    int&                finished;

    public:
        ActionBlocking(int port, std::string const& data, int& finished)
            : port(port)
            , data(data)
            , finished(finished)
        {}

        void stop()
        {
            finished = 1;
            // Connects forcing accept to finish
            ThorsAnvil::Socket::ConnectSocket   connect("127.0.0.1", port);
        }

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

