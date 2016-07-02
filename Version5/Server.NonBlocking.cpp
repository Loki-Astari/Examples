
#include "Socket.h"
#include "ProtocolHTTP.h"
#include "Utility.h"
#include "Common.h"
#include "CommonNonBlocking.h"
#include <event2/event.h>
#include <event2/event-config.h>
#include <boost/coroutine/all.hpp>
#include <sys/ioctl.h>

namespace Sock = ThorsAnvil::Socket;

#define THREAD_LIB 0
#if THREAD_LIB == 0
int setUpLibEventForThreading() {return 0;}
#elif THREAD_LIB == 1
int setUpLibEventForThreading() {return evthread_use_pthreads();}
#elif THREAD_LIB == 2
int setUpLibEventForThreading() {return evthread_use_windows_threads();}
#else
#error "Unknown Threading Library for lib event"
#endif

auto eventBaseDeleter = [](event_base* base){event_base_free(base);};
auto eventDeleter     = [](event* ev)       {event_free(ev);};
using EventBase       = std::unique_ptr<event_base, decltype(eventBaseDeleter)&>;
using Event           = std::unique_ptr<event, decltype(eventDeleter)&>;

EventBase   eventBase(nullptr, eventBaseDeleter);
Event       listener(nullptr, eventDeleter);
std::string data;
Action      action(data);

class HTTPProxy
{
    public:
        enum ConnectionPhase {Init, ReadPhase, WritePhase, Done};
    private:
        using PullType = boost::coroutines::asymmetric_coroutine<ConnectionPhase>::pull_type;
        using PushType = boost::coroutines::asymmetric_coroutine<ConnectionPhase>::push_type;

        class NonBlockingCoRoutine: public Sock::PolicyNonBlocking
        {
            PushType&   sink;
            public:
                NonBlockingCoRoutine(PushType& sink);
                virtual void setNonBlocking(int socketId) override;
                virtual void readWouldBlock()    override {sink(ReadPhase);}
                virtual void writeWouldBlock()   override {sink(WritePhase);}
        };
        Sock::ServerSocket& server;
        Action&             action;
        int                 socketId;
        ConnectionPhase     phase;
        EventBase&          eventBase;
        struct event*       event;
        PullType            source;

        void processesHttpRequest(PushType& sink);
        void setEventHandler();
    public:

        HTTPProxy(Sock::ServerSocket& server, Action& action, EventBase& eventBase);
        ~HTTPProxy();
        int             getSocketId()   const {return socketId;}
        ConnectionPhase getPhase()      const {return phase;}
        bool eventTrigger();
};

extern "C" void httpProxyCallback(evutil_socket_t /*fd*/, short /*event*/, void* acceptedCallBack)
{
    std::unique_ptr<HTTPProxy> proxy(reinterpret_cast<HTTPProxy*>(acceptedCallBack));

    if (proxy->eventTrigger())
    {
        // Still more to do.
        // So let the event loop take back ownership
        proxy.release();
    }
}

HTTPProxy::NonBlockingCoRoutine::NonBlockingCoRoutine(PushType& sink)
    : sink(sink)
{}

void HTTPProxy::NonBlockingCoRoutine::setNonBlocking(int socketId)
{
    int on = 1;
    if (setsockopt(socketId, SOL_SOCKET,  SO_REUSEADDR,(char *)&on, sizeof(on)) == -1)
    {
        close(socketId);
        throw std::logic_error(Sock::buildErrorMessage("HTTPProxy::NonBlockingCoRoutine::", __func__, ": setsockopt failed to make socket non blocking: ", strerror(errno)));
    }
    //Set socket to be non-blocking.
    if (ioctl(socketId, FIONBIO, (char *)&on) == -1)
    {
        close(socketId);
        throw std::logic_error(Sock::buildErrorMessage("HTTPProxy::NonBlockingCoRoutine::", __func__, ": ioctl failed to make socket non blocking: ", strerror(errno)));
    }
}

void HTTPProxy::processesHttpRequest(PushType& sink)
{
    NonBlockingCoRoutine    nonBlockingService(sink);
    Sock::DataSocket    accept(server.accept(nonBlockingService));

    socketId = accept.getSocketId();
    worker(std::move(accept), action);
}

HTTPProxy::HTTPProxy(Sock::ServerSocket& server, Action& action, EventBase& eventBase)
    : server(server)
    , action(action)
    , phase(Init)
    , eventBase(eventBase)
    , event(nullptr)
    , source([this](PushType& sink){return this->processesHttpRequest(sink);})
{
    phase   = source.get();
    if (phase != HTTPProxy::Done)
    {
        setEventHandler();
    }
}

void HTTPProxy::setEventHandler()
{
    short filter = (phase == HTTPProxy::ReadPhase) ? EV_READ : EV_WRITE;
    event = event_new(eventBase.get(), getSocketId(), filter | EV_PERSIST | EV_ET, httpProxyCallback, this);
    if (event == nullptr)
    {
        throw std::runtime_error(Sock::buildErrorMessage("HTTPProxy::", __func__, ": event_base_new Failed"));
    }
    if (event_add(event, nullptr) != 0)
    {
        throw std::runtime_error(Sock::buildErrorMessage("HTTPProxy::", __func__, ": event_add Failed"));
    }
}


HTTPProxy::~HTTPProxy()
{
    if (event != nullptr)
    {
        event_del(event);
        event_free(event);
    }
}

bool HTTPProxy::eventTrigger()
{
    if (!source())
    {
        return false;
    }

    ConnectionPhase newPhase = source.get();
    if (newPhase != phase)
    {
        phase = newPhase;
        event_del(event);
        event_free(event);
        setEventHandler();
    }
    return true;
}

extern "C" void serverCallback(evutil_socket_t /*fd*/, short /*event*/, void* serverCallBack)
{
    Sock::ServerSocket* server = reinterpret_cast<Sock::ServerSocket*>(serverCallBack);

    std::unique_ptr<HTTPProxy>  proxy(new HTTPProxy(*server, action, eventBase));
    HTTPProxy::ConnectionPhase phase = proxy->getPhase();
    if (phase != HTTPProxy::Done)
    {
        proxy.release();
    }
}

int main(int argc, char* argv[])
{
    using ThorsAnvil::Socket::buildErrorMessage;
    data     = Sock::commonSetUp(argc, argv);

    if (setUpLibEventForThreading() != 0)
    {
        throw std::runtime_error(buildErrorMessage("X::", __func__, ": setUpLibEventForThreading Failed"));
    }

    eventBase.reset(event_base_new());
    if (eventBase.get() == nullptr)
    {
        throw std::runtime_error(buildErrorMessage("X::", __func__, ": event_base_new Failed"));
    }
    Sock::ServerSocket   server(8086);
    listener.reset(event_new(eventBase.get(), server.getSocketId(), EV_READ | EV_PERSIST | EV_ET, serverCallback, &server));
    if (listener.get() == nullptr)
    {
        throw std::runtime_error(buildErrorMessage("X::", __func__, ": event_base_new Failed"));
    }
    if (event_add(listener.get(), nullptr) != 0)
    {
        throw std::runtime_error(buildErrorMessage("X::", __func__, ": event_add Failed"));
    }

    if (event_base_dispatch(eventBase.get()) == -1)
    {
        throw std::runtime_error(buildErrorMessage("X::", __func__, ": event_base_dispatch Failed"));
    }
}

