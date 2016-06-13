
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


class WorkJob
{
    Sock::DataSocket    accept;
    std::string const&  reply;
    public:
        WorkJob(Sock::DataSocket&& accept, std::string const& reply)
            : accept(std::move(accept))
            , reply(reply)
        {}
        void operator()()
        {
            Sock::HTTPServer  acceptHTTPServer(accept);
            try
            {
                std::string message;
                acceptHTTPServer.recvMessage(message);
                //std::cout << message << "\n";
                acceptHTTPServer.sendMessage("", reply);
            }
            catch(Sock::DropDisconnectedPipe const& e)
            {
                std::cerr << "Pipe Disconnected: " << e.what() << "\n";
            }
        }
};

class ThreadQueue
{
    using WorkList = std::deque<WorkJob>;

    std::vector<std::thread>    threads;
    std::mutex                  sync;
    std::condition_variable     cond;

    WorkList                    work;

    WorkJob getWorkJob()
    {
        std::unique_lock<std::mutex>     lock(sync);
        cond.wait(lock, [this](){return !this->work.empty();});

        WorkJob result = std::move(work.front());
        work.pop_front();
        return result;
    }
    void doWork()
    {
        while(true)
        {
            WorkJob job = getWorkJob();
            job();
        }
    }

    public:
        void startJob(WorkJob&& item)
        {
            std::unique_lock<std::mutex>     lock(sync);
            work.push_back(std::move(item));
            cond.notify_one();
        }
        ThreadQueue(int count)
            : threads(count)
        {
            for(int loop = 0;loop < count; ++loop)
            {
                threads[loop] = std::thread(&ThreadQueue::doWork, this);
            }
        }
};


int main(int argc, char* argv[])
{
    std::string data    = Sock::commonSetUp(argc, argv);
    Sock::ServerSocket   server(8080);
    int                  finished    = 0;

    ThreadQueue     jobs(3);

    while(!finished)
    {
        Sock::DataSocket  accept  = server.accept();

        jobs.startJob(WorkJob(std::move(accept), data));
    }
}
