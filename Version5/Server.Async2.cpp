#include "Socket.h"
#include "Common.h"
#include "CommonBlocking.h"
#include <string>
#include <list>
#include <future>
#include <mutex>
#include <memory>
#include <utility>

namespace Sock = ThorsAnvil::Socket;

class FutureQueue
{
    using MyFuture   = std::future<void>;
    using FutureList = std::list<MyFuture>;

    FutureList                  futures;
    std::mutex                  mutex;

    public:
        template<typename T>
        void addFuture(T&& lambda)
        {
            std::unique_lock<std::mutex> lock(mutex);
            FutureList::iterator future = futures.insert(futures.end(), std::future<void>());
            (*future)    = std::async(std::launch::async, [this, future, lambda = std::move(lambda)]() mutable
                                        {
                                            lambda();
                                            (*future).wait();
                                            std::unique_lock<std::mutex> lock(this->mutex);
                                            this->futures.erase(future);
                                        }
                                     );
        }
};

int main(int argc, char* argv[])
{
    std::string         data     = Sock::commonSetUp(argc, argv);
    int                 finished = 0;
    Sock::ServerSocket  server(8085);
    ActionBlocking      action(8085, data, finished);
    FutureQueue         future;


    while (!finished)
    {
        Sock::DataSocket  accept  = server.accept();

        future.addFuture([accept = std::move(accept), &action]() mutable {Sock::worker(std::move(accept), action);});
    }
}
