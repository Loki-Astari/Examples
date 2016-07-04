#include "CommonNonBlocking.h"
#include "EventLoop.h"
#include <event2/event.h>
#include <iostream>

ActionNonBlocking::ActionNonBlocking(ThorsAnvil::Socket::EventLoop& loop, std::string const& data)
    : loop(loop)
    , data(data)
{}

void ActionNonBlocking::stop()
{
     event_base_loopbreak(loop);
}

void ActionNonBlocking::log(std::string const& message)
{
    std::cerr << message << "\n";
}
