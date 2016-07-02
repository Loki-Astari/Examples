
#ifndef THORSANVIL_SOCKET_SOCKET_H
#define THORSANVIL_SOCKET_SOCKET_H


#include <string>
#include <vector>
#include <sstream>
#include <stdexcept>
#include <cstring>

#if defined(EWOULDBLOCK_DEFINED_AND_DIFFERENT_FROM_EAGAIN)
#define CASE_EWOULDBLOCK   case EWOULDBLOCK:
#else
#define CASE_EWOULDBLOCK
#endif

namespace ThorsAnvil
{
    namespace Socket
    {

class DropDisconnectedPipe: public std::runtime_error
{
    using std::runtime_error::runtime_error;
};

class PolicyNonBlocking
{
    public:
        virtual ~PolicyNonBlocking()                    {}
        virtual void setNonBlocking(int /*socketId*/)   {}
        virtual void readWouldBlock()                   {}
        virtual void writeWouldBlock()                  {}
        virtual void readTimeout()                      {}
        virtual void writeTimeout()                     {}
};

class PolicyInterupt
{
    public:
        virtual ~PolicyInterupt()                       {}
        virtual void interuptTriggered()                {}
};

// An RAII base class for handling sockets.
// Socket is movable but not copyable.
class BaseSocket
{
    private:
        int    socketId;
    protected:
        static PolicyNonBlocking defaultPolicyForNonBlocking;
        static PolicyInterupt    defaultPolicyInterupt;
    protected:
        static constexpr int invalidSocketId      = -1;

        // Designed to be a base class not used used directly.
        BaseSocket(int socketId);
    public:
        int getSocketId() const {return socketId;}
    public:
        virtual ~BaseSocket();

        // Moveable but not Copyable
        BaseSocket(BaseSocket&& move)               noexcept;
        BaseSocket& operator=(BaseSocket&& move)    noexcept;
        void swap(BaseSocket& other)                noexcept;
        BaseSocket(BaseSocket const&)               = delete;
        BaseSocket& operator=(BaseSocket const&)    = delete;

        // User can manually call close
        void close();
};

// A class that can read/write to a socket
class DataSocket: public BaseSocket
{
    PolicyNonBlocking&     nonBlockingPolicy;
    PolicyInterupt&        interuptPolicy;
    public:
        DataSocket(int socketId,
                   PolicyNonBlocking& nonBlockingPolicy = defaultPolicyForNonBlocking,
                   PolicyInterupt&    interuptPolicy    = defaultPolicyInterupt)
            : BaseSocket(socketId)
            , nonBlockingPolicy(nonBlockingPolicy)
            , interuptPolicy(interuptPolicy)
        {
            nonBlockingPolicy.setNonBlocking(getSocketId());
        }

        template<typename F>
        std::size_t getMessageData(char* buffer, std::size_t size, F scanForEnd = [](std::size_t){return false;});
        void        putMessageData(char const* buffer, std::size_t size);
        void        putMessageClose();
};

// A class the conects to a remote machine
// Allows read/write accesses to the remote machine
class ConnectSocket: public DataSocket
{
    public:
        ConnectSocket(std::string const& host, int port,
                      PolicyNonBlocking& nonBlockingPolicy = defaultPolicyForNonBlocking,
                      PolicyInterupt&    interuptPolicy    = defaultPolicyInterupt);
};

// A server socket that listens on a port for a connection
class ServerSocket: public BaseSocket
{
    static constexpr int maxConnectionBacklog = 255;
    public:
        ServerSocket(int port);

        // An accepts waits for a connection and returns a socket
        // object that can be used by the client for communication
        DataSocket accept(PolicyNonBlocking& nonBlockingPolicy = defaultPolicyForNonBlocking,
                          PolicyInterupt&    interuptPolicy    = defaultPolicyInterupt);
};

    }
}

#include "Socket.tpp"

#endif

