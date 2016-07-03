
#include "Socket.h"
#include "ProtocolHTTP.h"
#include "Common.h"
#include "CommonBlocking.h"
#include <list>
#include <iostream>
#include <future>
#include <mutex>
#include <condition_variable>

namespace Sock = ThorsAnvil::Socket;

class FutureQueue
{
    using MyFuture   = std::future<void>;
    using FutureList = std::list<MyFuture>;

    int&                        finished;
    FutureList                  futures;
    std::mutex                  mutex;
    std::condition_variable     cond;
    std::thread                 cleaner;

    void waiter()
    {
        while(finished)
        {
            std::future<void>   next;
            {
                std::unique_lock<std::mutex> lock(mutex);
                cond.wait(lock, [this](){return !(this->futures.empty() && !this->finished);});
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
    public:
        FutureQueue(int& finished)
            : finished(finished)
            , cleaner(&FutureQueue::waiter, this)
        {}
        ~FutureQueue()
        {
            cleaner.join();
        }

        template<typename T>
        void addFuture(T&& lambda)
        {
            std::unique_lock<std::mutex> lock(mutex);
            futures.push_back(std::async(std::launch::async, std::move(lambda)));
            cond.notify_one();
        }
};

int main(int argc, char* argv[])
{
    std::string         data     = Sock::commonSetUp(argc, argv);
    int                 finished = 0;
    Sock::ServerSocket  server(8084);
    ActionBlocking      action(8084, data, finished);
    FutureQueue         future(finished);


    while(!finished)
    {
        Sock::DataSocket  accept  = server.accept();

        future.addFuture([accept = std::move(accept), &action]() mutable {Sock::worker<ActionBlocking>(std::move(accept), action);});
    }
}
