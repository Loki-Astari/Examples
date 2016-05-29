
#ifndef THORSANVIL_SOCKET_H
#define THORSANVIL_SOCKET_H

#include <string>
#include <sstream>

namespace ThorsAnvil
{
    namespace Socket
    {

template<std::size_t I = 0, typename... Tp>
inline typename std::enable_if<I == sizeof...(Tp), void>::type
print(std::ostream& s, std::tuple<Tp...> const& t)
{ }

template<std::size_t I = 0, typename... Tp>
inline typename std::enable_if<I < sizeof...(Tp), void>::type
print(std::ostream& s, std::tuple<Tp...> const& t)
{
    s << std::get<I>(t);
    print<I + 1, Tp...>(s, t);
}

template<typename... Args>
std::string buildErrorMessage(Args const&... args)
{
    std::stringstream msg;
    print(msg, std::make_tuple(args...));
    return msg.str();
}

// An RAII base class for handling sockets.
// Socket is movable but not copyable.
class BaseSocket
{
    int     socketId;
    protected:
        // Designed to be a base class not used used directly.
        BaseSocket(int socketId);
        int getSocketId() const {return socketId;}
    public:
        ~BaseSocket();

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
        DataSocket(int socketId);

        bool getMessage(std::string& message);
        void putMessage(std::string const& message);
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
    public:
        ServerSocket(int port);

        // An accepts waits for a connection and returns a socket
        // object that can be used by the client for communication
        DataSocket accept();
};

    }
}

#endif

