
#include "Socket.h"
#include "ProtocolHTTP.h"
#include "Common.h"
#include <list>
#include <iostream>
#include <future>
#include <mutex>
#include <condition_variable>

namespace Sock = ThorsAnvil::Socket;

int main(int argc, char* argv[])
{
    using FutureList = std::list<std::future<void>>;

    FutureList                  futures;
    std::mutex                  mutex;
    std::condition_variable     cond;

    std::string data    = Sock::commonSetUp(argc, argv);
    Sock::ServerSocket   server(8080);
    int                  finished    = 0;

    std::thread         waiter([&finished, &futures, &mutex, &cond]()
        {
            while(finished)
            {
                std::future<void>   next;
                {
                    std::unique_lock<std::mutex> lock(mutex);
                    cond.wait(lock, [&finished, &futures](){return !(futures.empty() && !finished);});
                    if (futures.empty() && !finished)
                    {
                        next = std::move(futures.front());
                        futures.pop_front();
                    }
                }
                if (!next.valid())
                {
                    next.wait();
                }
            }

        }
    );


    while(!finished)
    {
        Sock::DataSocket  accept  = server.accept();
        std::unique_lock<std::mutex> lock(mutex);
        futures.push_back(std::async([accept = std::move(accept), &data]() mutable
            {
                Sock::HTTPServer  acceptHTTPServer(accept);
                try
                {
                    std::string message;
                    acceptHTTPServer.recvMessage(message);
                    //std::cout << message << "\n";

                    if (message == "Done")
                    {
                        finished = 1;
                        server.stop();
                        acceptHTTPServer.sendMessage("", "Stoped");
                    }
                    else
                    {
                        acceptHTTPServer.sendMessage("", data);
                    }
                }
                catch(Sock::DropDisconnectedPipe const& e)
                {
                    std::cerr << "Pipe Disconnected: " << e.what() << "\n";
                }
            })
        );
        cond.notify_one();
    }
}
