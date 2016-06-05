
#ifndef THORSANVIL_SOCKET_SOCKET_H
#define THORSANVIL_SOCKET_SOCKET_H

#include <string>
#include <vector>
#include <sstream>

namespace ThorsAnvil
{
    namespace Socket
    {

// An RAII base class for handling sockets.
// Socket is movable but not copyable.
class BaseSocket
{

    int     socketId;
    protected:
        static constexpr int invalidSocketId      = -1;

        // Designed to be a base class not used used directly.
        BaseSocket(int socketId);
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
    public:
        DataSocket(int socketId)
            : BaseSocket(socketId)
        {}

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
        ConnectSocket(std::string const& host, int port);
};

// A server socket that listens on a port for a connection
class ServerSocket: public BaseSocket
{
    static constexpr int maxConnectionBacklog = 5;
    public:
        ServerSocket(int port);

        // An accepts waits for a connection and returns a socket
        // object that can be used by the client for communication
        DataSocket accept();
};

    }
}

#include "Socket.tpp"

#endif

