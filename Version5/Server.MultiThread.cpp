
#include "Socket.h"
#include "ProtocolHTTP.h"
#include "Common.h"
#include <iostream>
#include <list>
#include <thread>
#include <mutex>

namespace Sock = ThorsAnvil::Socket;

class ThreadHolder
{
    using ThreadList        = std::list<std::thread>;
    using ThreadListIter    = typename ThreadList::iterator;
    using OldList           = std::list<ThreadListIter>;

    std::mutex      guard;
    ThreadList      backGroundJobs;
    OldList         deadJobMarkers;

    public:
        ~ThreadHolder()
        {
            cleanList();
            for(auto& job: backGroundJobs)
            {
                job.join();
            }
        }
        void cleanList()
        {
            std::lock_guard<std::mutex> lock(guard);
            for(auto deadJob: deadJobMarkers)
            {
                deadJob->join();
                backGroundJobs.erase(deadJob);
            }
            deadJobMarkers.clear();
        }
        template<typename F>
        void startJob(F&& job)
        {
            runJob(std::move(job));
            cleanList();
        }
        template<typename F>
        void runJob(F&& job)
        {
            std::lock_guard<std::mutex> lock(guard);
            auto location = backGroundJobs.emplace(backGroundJobs.end());
            backGroundJobs.back() = std::thread([this, location, job = std::move(job)]() mutable
                {
                    job();
                    std::lock_guard<std::mutex> lock(this->guard);
                    this->deadJobMarkers.push_back(location);
                }
            );
        }
};


int main(int argc, char* argv[])
{
    std::string     data    = Sock::commonSetUp(argc, argv);
    ThreadHolder    jobs;

    Sock::ServerSocket   server(8080);
    int                  finished    = 0;

    while(!finished)
    {
        Sock::DataSocket  accept  = server.accept();

        jobs.startJob([accept = std::move(accept), &server, &data, &finished]() mutable
            {
                Sock::HTTPServer  acceptHTTPServer(accept);
                try
                {
                    std::string message;
                    acceptHTTPServer.recvMessage(message);
                    // std::cout << message << "\n";
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
            }
        );
    }
}
