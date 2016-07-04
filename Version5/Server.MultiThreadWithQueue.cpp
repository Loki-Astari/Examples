#include "Socket.h"
#include "Common.h"
#include "CommonBlocking.h"
#include <vector>
#include <deque>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <iostream>
#include <utility>

namespace Sock = ThorsAnvil::Socket;


// Needed because std::function<> is not movable.
class WorkJob
{
    Sock::DataSocket    accept;
    ActionBlocking&     action;
    public:
        WorkJob(Sock::DataSocket&& accept, ActionBlocking& action)
            : accept(std::move(accept))
            , action(action)
        {}
        WorkJob(WorkJob&& rhs)
            : accept(std::move(rhs.accept))
            , action(rhs.action)
        {}
        void operator()()
        {
            Sock::worker(std::move(accept), action);
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
        while (!finished)
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
            for (int loop = 0;loop < count; ++loop)
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
    std::string         data     = Sock::commonSetUp(argc, argv);
    int                 finished = 0;
    Sock::ServerSocket  server(PORT);
    ActionBlocking      action(PORT, data, finished);

    std::cerr << "Concurrency: " << std::thread::hardware_concurrency() << "\n";
    ThreadQueue     jobs(std::thread::hardware_concurrency() * POOL_MULTI);

    while (!finished)
    {
        Sock::DataSocket  accept  = server.accept();

        jobs.startJob(WorkJob(std::move(accept), action));
    }
}
