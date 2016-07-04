#include "CommonBlocking.h"
#include "Socket.h"
#include <iostream>

ActionBlocking::ActionBlocking(int port, std::string const& data, int& finished)
    : port(port)
    , data(data)
    , finished(finished)
{}

void ActionBlocking::stop()
{
    finished = 1;
    // Connects forcing accept to finish
    ThorsAnvil::Socket::ConnectSocket   connect("127.0.0.1", port);
}

void ActionBlocking::log(std::string const& message)
{
    std::cerr << message << "\n";
}
