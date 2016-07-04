
#include "EventLoop.h"
#include "Common.h"
#include <sys/ioctl.h>
#include <stdexcept>
#include <utility>

namespace ThorsAnvil
{
    namespace Socket
    {
void callbackEventClient(int /*fd*/, short /*event*/, void* cbData)
{
    namespace Sock = ThorsAnvil::Socket;
    std::unique_ptr<Sock::EventClient> client(reinterpret_cast<Sock::EventClient*>(cbData));
    if (client->handleEvent())
    {
        client.release();
    }
}

void callbackEventServer(int /*fd*/, short /*event*/, void* cbData)
{
    namespace Sock = ThorsAnvil::Socket;
    Sock::EventServer* server = reinterpret_cast<Sock::EventServer*>(cbData);
    server->handleEvent();
}
    }
}

using namespace ThorsAnvil::Socket;

EventLoop::EventLoop()
    : eventBase(event_base_new(), [](EventBase* base) {event_base_free(base);})
{
    if (eventBase.get() == nullptr)
    {
        throw std::runtime_error(buildErrorMessage("EventLoop::", __func__, ": event_base_new Failed"));
    }
}

void EventLoop::runLoop()
{
    if (event_base_dispatch(eventBase.get()) == -1)
    {
        throw std::runtime_error(buildErrorMessage("EventLoop::", __func__, ": event_base_dispatch Failed"));
    }
}

void EventClient::NonBlockingPolicy::setNonBlocking(int socketId)
{
    int on = 1;
    if (setsockopt(socketId, SOL_SOCKET,  SO_REUSEADDR, reinterpret_cast<char*>(&on), sizeof(on)) == -1)
    {
        close(socketId);
        throw std::logic_error(buildErrorMessage("EventClient::NonBlockingPolicy::", __func__, ": setsockopt failed to make socket non blocking: ", strerror(errno)));
    }
    //Set socket to be non-blocking.
    if (ioctl(socketId, FIONBIO, reinterpret_cast<char*>(&on)) == -1)
    {
        close(socketId);
        throw std::logic_error(buildErrorMessage("EventClient::NonBlockingPolicy::", __func__, ": ioctl failed to make socket non blocking: ", strerror(errno)));
    }
}

void EventClient::NonBlockingPolicy::readWouldBlock()
{
    (*sinkPtr)(Read);
}

void EventClient::NonBlockingPolicy::writeWouldBlock()
{
    (*sinkPtr)(Write);
}

void EventClient::processesHttpRequest(PushType& sink)
{
    namespace Sock = ThorsAnvil::Socket;

    nonBlockingPolicy.sinkPtr = &sink;
    sink(Read);
    Sock::worker(std::move(connection), action);
    sink(Done);
}

void EventClient::setUpEventLoop()
{
    short filter = (phase == Read) ? EV_READ : EV_WRITE;
    eventObj.reset(event_new(loop, connection.getSocketId(), filter | EV_PERSIST | EV_ET, callbackEventClient, this));
    if (eventObj == nullptr)
    {
        throw std::runtime_error(buildErrorMessage("EventClient::", __func__, ": event_base_new Failed"));
    }
    if (event_add(eventObj.get(), nullptr) != 0)
    {
        throw std::runtime_error(buildErrorMessage("EventClient::", __func__, ": event_add Failed"));
    }
}

EventClient::EventClient(EventLoop& loop, ServerSocket& server, ActionNonBlocking& action)
    : loop(loop)
    , action(action)
    , connection(server.accept(nonBlockingPolicy))
    , phase(Created)
    , eventObj(nullptr, [](Event* ev) {event_del(ev);event_free(ev);})
    , coRoutineHandler([this](PushType& sink){return this->processesHttpRequest(sink);})
{
    phase   = coRoutineHandler.get();
    if (phase != Done)
    {
        setUpEventLoop();
    }
}

bool EventClient::handleEvent()
{
    coRoutineHandler();
    ConnectionPhase newPhase = coRoutineHandler.get();
    if (newPhase != phase)
    {
        phase = newPhase;
        if (phase != Done)
        {
            setUpEventLoop();
        }
    }
    return phase != Done;
}

EventServer::EventServer(int port, EventLoop& loop, ActionNonBlocking& action)
    : loop(loop)
    , action(action)
    , server(port)
    , eventObj(event_new(loop, server.getSocketId(), EV_READ | EV_PERSIST | EV_ET, callbackEventServer, this), [](Event* ev) {event_del(ev);event_free(ev);})
{
    if (eventObj.get() == nullptr)
    {
        throw std::runtime_error(buildErrorMessage("EventServer::", __func__, ": event_base_new Failed"));
    }
    if (event_add(eventObj.get(), nullptr) != 0)
    {
        throw std::runtime_error(buildErrorMessage("EventServer::", __func__, ": event_add Failed"));
    }
}

void EventServer::handleEvent()
{
    std::unique_ptr<EventClient> proxy(new EventClient(loop, server, action));
    EventClient::ConnectionPhase phase = proxy->getPhase();
    if (phase != EventClient::Done)
    {
        proxy.release();
    }
}

