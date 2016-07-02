
#include "Common.h"
#include <event2/event.h>
#include <boost/coroutine/all.hpp>
#include <sys/ioctl.h>

std::string data;

namespace ThorsAnvil
{
    namespace Socket
    {

using EventBase = struct event_base;
using Event     = struct event;

auto eventBaseDeleter = [](EventBase* base) {event_base_free(base);};
auto eventDeleter     = [](Event* ev)       {event_del(ev);event_free(ev);};

class EventLoop
{
        using EventBasePtr    = std::unique_ptr<event_base, decltype(eventBaseDeleter)&>;

        EventBasePtr    eventBase;
        
    public:
        EventLoop()
            : eventBase(event_base_new(), eventBaseDeleter)
        {
            if (eventBase.get() == nullptr)
            {
                throw std::runtime_error(buildErrorMessage("EventLoop::", __func__, ": event_base_new Failed"));
            }
        }
        operator EventBase*() const {return eventBase.get();}

        void runLoop()
        {
            if (event_base_dispatch(eventBase.get()) == -1)
            {
                throw std::runtime_error(buildErrorMessage("EventLoop::", __func__, ": event_base_dispatch Failed"));
            }
        }
};


extern "C" void callbackEventClient(int fd, short event, void* cbData);
extern "C" void callbackEventServer(int fd, short event, void* cbData);

class EventClient
{
    public:
        enum ConnectionPhase {Created, Read, Write, Done};
    private:
        using PullType = boost::coroutines::asymmetric_coroutine<ConnectionPhase>::pull_type;
        using PushType = boost::coroutines::asymmetric_coroutine<ConnectionPhase>::push_type;
        using EventPtr = std::unique_ptr<Event, decltype(eventDeleter)>;

        struct NonBlockingPolicy: public PolicyNonBlocking
        {
            PushType*   sinkPtr = nullptr;

            virtual void setNonBlocking(int socketId) override
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
            virtual void readWouldBlock() override
            {
                (*sinkPtr)(Read);
            }
            virtual void writeWouldBlock() override
            {
                (*sinkPtr)(Write);
            }
        };
        EventLoop&          loop;
        ServerSocket&       server;
        DataSocket          connection;
        int&                finished;
        ConnectionPhase     phase;

        NonBlockingPolicy   nonBlockingPolicy;
        EventPtr            event;
        PullType            coRoutineHandler;

        void processesHttpRequest(PushType& sink)
        {
            nonBlockingPolicy.sinkPtr = &sink;
            sink(Read);
            worker(std::move(connection), server, data, finished);
            sink(Done);
        }
        void setUpEventLoop()
        {
            short filter = (phase == Read) ? EV_READ : EV_WRITE;
            event.reset(event_new(loop, connection.getSocketId(), filter | EV_PERSIST | EV_ET, callbackEventClient, this));
            if (event == nullptr)
            {
                throw std::runtime_error(buildErrorMessage("EventClient::", __func__, ": event_base_new Failed"));
            }
            if (event_add(event.get(), nullptr) != 0)
            {
                throw std::runtime_error(buildErrorMessage("EventClient::", __func__, ": event_add Failed"));
            }
        }

    public:

        EventClient(EventLoop& loop, ServerSocket& server, int& finished)
            : loop(loop)
            , server(server)
            , connection(server.accept(nonBlockingPolicy))
            , finished(finished)
            , phase(Created)
            , event(nullptr, eventDeleter)
            , coRoutineHandler([this](PushType& sink){return this->processesHttpRequest(sink);})
        {
            phase   = coRoutineHandler.get();
            if (phase != Done)
            {
                setUpEventLoop();
            }
        }

        ConnectionPhase     getPhase() const {return phase;}

    private:
        friend void callbackEventClient(int fd, short event, void* cbData);
        bool handleEvent()
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
};

void callbackEventClient(int /*fd*/, short /*event*/, void* cbData)
{
    namespace Sock = ThorsAnvil::Socket;
    std::unique_ptr<Sock::EventClient> client(reinterpret_cast<Sock::EventClient*>(cbData));
    if (client->handleEvent())
    {
        client.release();
    }
}

class EventServer
{
    private:
        using EventPtr          = std::unique_ptr<event, decltype(eventDeleter)&>;

        EventLoop&      loop;
        ServerSocket    server;
        EventPtr        event;

    public:
        EventServer(int port, EventLoop& loop)
            : loop(loop)
            , server(port)
            , event(event_new(loop, server.getSocketId(), EV_READ | EV_PERSIST | EV_ET, callbackEventServer, this), eventDeleter)
        {
            if (event.get() == nullptr)
            {
                throw std::runtime_error(buildErrorMessage("EventServer::", __func__, ": event_base_new Failed"));
            }
            if (event_add(event.get(), nullptr) != 0)
            {
                throw std::runtime_error(buildErrorMessage("EventServer::", __func__, ": event_add Failed"));
            }
        }
    private:
        friend void callbackEventServer(int fd, short event, void* cbData);
        void handleEvent()
        {
            static int finished = 0;

            std::unique_ptr<EventClient> proxy(new EventClient(loop, server, finished));
            EventClient::ConnectionPhase phase = proxy->getPhase();
            if (phase != EventClient::Done)
            {
                proxy.release();
            }
        }
};

void callbackEventServer(int /*fd*/, short /*event*/, void* cbData)
{
    namespace Sock = ThorsAnvil::Socket;
    Sock::EventServer* server = reinterpret_cast<Sock::EventServer*>(cbData);
    server->handleEvent();
}

    }
}

int main(int argc, char* argv[])
{
    namespace Sock = ThorsAnvil::Socket;

    data = Sock::commonSetUp(argc, argv);

    Sock::EventLoop     eventLoop;
    Sock::EventServer   server(8087, eventLoop);

    eventLoop.runLoop();
}

