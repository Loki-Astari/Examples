
#include "Socket.h"
#include "ProtocolHTTP.h"
#include "Common.h"
#include <iostream>
#include <list>
#include <deque>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace Sock = ThorsAnvil::Socket;


// Needed because std::function<> is not movable.
class WorkJob
{
    Sock::DataSocket    accept;
    Sock::ServerSocket& server;
    std::string const&  data;
    int&                finished;
    public:
        WorkJob(Sock::DataSocket&& accept, Sock::ServerSocket& server, std::string const& data, int& finished)
            : accept(std::move(accept))
            , server(server)
            , data(data)
            , finished(finished)
        {}
        WorkJob(WorkJob&& rhs)
            : accept(std::move(rhs.accept))
            , server(rhs.server)
            , data(rhs.data)
            , finished(rhs.finished)
        {}
        void operator()()
        {
            Sock::worker(std::move(accept), server, data, finished);
        }
};

class ThreadQueue
{
    using WorkList = std::deque<WorkJob>;

    std::vector<std::thread>    threads;
    std::mutex                  safe;
    std::condition_variable     cond;
    WorkList                    work;
    int                         finished;

    WorkJob getWorkJob()
    {
        std::unique_lock<std::mutex>     lock(safe);
        cond.wait(lock, [this](){return !(this->work.empty() && !this->finished);});

        auto result = std::move(work.front());
        work.pop_front();
        return result;
    }
    void doWork()
    {
        while(!finished)
        {
            WorkJob job = getWorkJob();
            if (!finished)
            {
                job();
            }
        }
    }

    public:
        void startJob(WorkJob&& item)
        {
            std::unique_lock<std::mutex>     lock(safe);
            work.push_back(std::move(item));
            cond.notify_one();
        }

        ThreadQueue(int count)
            : threads(count)
            , finished(false)
        {
            for(int loop = 0;loop < count; ++loop)
            {
                threads[loop] = std::thread(&ThreadQueue::doWork, this);
            }
        }
        ~ThreadQueue()
        {
            {
                std::unique_lock<std::mutex>     lock(safe);
                finished = true;
            }
            cond.notify_all();
        }
};

int main(int argc, char* argv[])
{
    std::string data    = Sock::commonSetUp(argc, argv);
    Sock::ServerSocket   server(PORT);
    int                  finished    = 0;

    std::cerr << "Concurrency: " << std::thread::hardware_concurrency() << "\n";
    ThreadQueue     jobs(std::thread::hardware_concurrency() * POOL_MULTI);

    while(!finished)
    {
        Sock::DataSocket  accept  = server.accept();

        jobs.startJob(WorkJob(std::move(accept), server, data, finished));
    }
}
