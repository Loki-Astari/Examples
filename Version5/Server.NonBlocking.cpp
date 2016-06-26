
#include "Socket.h"
#include "ProtocolHTTP.h"
#include "Utility.h"
#include "Common.h"
#include <event2/event.h>
#include <event2/event-config.h>

namespace Sock = ThorsAnvil::Socket;

int oldmain(int argc, char* argv[])
{
    std::string data    = Sock::commonSetUp(argc, argv);

    Sock::ServerSocket   server(8080);
    int                  finished    = 0;
    while(!finished)
    {
        Sock::DataSocket  accept  = server.accept();
        worker(std::move(accept), server, data, finished);
    }
    return 0;
}
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


void serverCallback(evutil_socket_t fd, short event, void* serverCallBack)
{
    std::cerr << "Read: " << EV_READ << " Write: " << EV_WRITE << " Event: " << event << " FD: " << fd << "\n";
    Sock::ServerSocket* server = reinterpret_cast<Sock::ServerSocket*>(serverCallBack);

    Sock::DataSocket  accept  = server->accept();

}

int main()
{
    using ThorsAnvil::Socket::buildErrorMessage;
    auto eventBaseDeleter = [](event_base* base){event_base_free(base);};
    auto eventDeleter     = [](event* ev)       {event_free(ev);};

    if (setUpLibEventForThreading() != 0)
    {
        throw std::runtime_error(buildErrorMessage("X::", __func__, ": setUpLibEventForThreading Failed"));
    }

    std::unique_ptr<event_base, decltype(eventBaseDeleter)>     eventBase(event_base_new() , eventBaseDeleter);
    if (eventBase.get() == nullptr)
    {
        throw std::runtime_error(buildErrorMessage("X::", __func__, ": event_base_new Failed"));
    }
    Sock::ServerSocket   server(8080);
    std::unique_ptr<event, decltype(eventDeleter)>              listener(event_new(eventBase.get(), server.getSocketId(), EV_READ | EV_PERSIST | EV_ET, serverCallback, &server), eventDeleter);
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

    // event_free(listener);
     // event_base_free(eventBase);
}

