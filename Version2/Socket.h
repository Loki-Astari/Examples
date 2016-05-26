
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

class SocketBase
{
    protected:
        int     socketId;
    public:
        SocketBase(int socketId);
        ~SocketBase();

        SocketBase(SocketBase&& move)               noexcept;
        SocketBase& operator=(SocketBase&& move)    noexcept;
        void swap(SocketBase& other)                noexcept;
        SocketBase(SocketBase const&)               = delete;
        SocketBase& operator=(SocketBase const&)    = delete;

        void close();
};

class DataSocket: public SocketBase
{
    public:
        using SocketBase::SocketBase;

        bool getMessage(std::string& message);
        void putMessage(std::string const& message);
};

class ConnectSocket: public DataSocket
{
    public:
        ConnectSocket(std::string const& host, int port);
};

class ServerSocket: public SocketBase
{
    public:
        ServerSocket(int port);

        DataSocket accept();
};

    }
}

#endif

