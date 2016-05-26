
#include "Socket.h"
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sstream>
#include <stdexcept>

using namespace ThorsAnvil::Socket;

SocketBase::SocketBase(int socketId)
    : socketId(socketId)
{
    if (socketId == -1)
    {
        throw std::runtime_error(buildErrorMessage("SocketBase::SocketBase: bad socket: ", strerror(errno)));
    }
}

SocketBase::~SocketBase()
{
    close();
}

void SocketBase::close()
{
    socketId = 0;   
    ::close(socketId);
}

void SocketBase::swap(SocketBase& other) noexcept
{
    using std::swap;
    swap(socketId,   other.socketId);
}

SocketBase::SocketBase(SocketBase&& move) noexcept
    : socketId(0)
{
    move.swap(*this);
}

SocketBase& SocketBase::operator=(SocketBase&& move) noexcept
{
    move.swap(*this);
    return *this;
}

ConnectSocket::ConnectSocket(std::string const& host, int port)
    : SocketData(::socket(PF_INET, SOCK_STREAM, 0))
{
    struct sockaddr_in serverAddr;
    bzero((char*)&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family       = AF_INET;
    serverAddr.sin_port         = htons(port);
    serverAddr.sin_addr.s_addr  = inet_addr(host.c_str());

    if (::connect(socketId, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) != 0)
    {
        close();
        throw std::runtime_error(buildErrorMessage("ConnectSocket::ConnectSocket: connect: ", strerror(errno)));
    }
}

ServerSocket::ServerSocket(int port)
    : SocketBase(::socket(PF_INET, SOCK_STREAM, 0))
{
    struct sockaddr_in serverAddr;
    bzero((char*)&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family       = AF_INET;
    serverAddr.sin_port         = htons(port);
    serverAddr.sin_addr.s_addr  = INADDR_ANY;

    if (::bind(socketId, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) != 0)
    {
        close();
        throw std::runtime_error(buildErrorMessage("ServerSocket::ServerSocket: bind: ", strerror(errno)));
    }

    if (::listen(socketId, 5) != 0)
    {
        close();
        throw std::runtime_error(buildErrorMessage("ServerSocket::ServerSocket: listen: ", strerror(errno)));
    }
}

SocketData ServerSocket::accept()
{
    struct  sockaddr_storage    serverStorage;
    socklen_t                   addr_size   = sizeof serverStorage;
    int newSocket = ::accept(socketId, (struct sockaddr*)&serverStorage, &addr_size);
    if (newSocket == -1)
    {
        throw std::runtime_error(buildErrorMessage("ServerSocket:accept: accept: ", strerror(errno)));
    }
    return SocketData(newSocket);
}

class StringSizer
{
    std::string&    stringData;
    std::size_t&    currentSize;
    public:
        StringSizer(std::string& stringData, std::size_t& currentSize)
            : stringData(stringData)
            , currentSize(currentSize)
        {
            stringData.resize(stringData.capacity());
        }
        ~StringSizer()
        {
            stringData.resize(currentSize);
        }
        void incrementSize(std::size_t amount)
        {
            currentSize += amount;
        }
};

bool SocketData::getMessage(std::string& message)
{
    std::size_t     dataRead = 0;

    while(true)
    {
        // This outer loop handles resizing of the message when we run of space in the string.

        StringSizer        stringSizer(message, dataRead);
        std::size_t const  capacity = message.capacity();
        std::size_t const  dataMax  = capacity - 1;
        char*              buffer   = &message[0];

        while(dataRead < dataMax)
        {
            // The inner loop handles interactions with the socket.
            std::size_t get = read(socketId, buffer + dataRead, dataMax - dataRead);
            if (get == -1)
            {
                switch(errno)
                {
                    case EBADF:
                    case EFAULT:
                    case EINVAL:
                    case ENXIO:
                    {
                        // Fatal error. Programming bug
                        throw std::domain_error(buildErrorMessage("SocketData::getMessage: read: critical error: ", strerror(errno)));
                    }
                    case EIO:
                    case ENOBUFS:
                    case ENOMEM:
                    {
                       // Resource acquisition failure or device error
                        throw std::runtime_error(buildErrorMessage("SocketData::getMessage: read: resource failure: ", strerror(errno)));
                    }
                    case ETIMEDOUT:
                    case EAGAIN:
                    case EINTR:
                    {
                        // Temporrary error.
                        // Simply retry the read.
                        continue;
                    }
                    case ECONNRESET:
                    case ENOTCONN:
                    {
                        // Connection broken.
                        // Return the data we have available and exit
                        // as if the connection was closed correctly.
                        get = 0;
                        break;
                    }
                    default:
                    {
                        throw std::runtime_error(buildErrorMessage("SocketData::getMessage: read: returned -1: ", strerror(errno)));
                    }
                }
            }
            if (get == 0)
            {
                return dataRead > 0;
            }
            dataRead += get;
            stringSizer.incrementSize(get);
        }
        message.reserve(message.capacity() * 1.5);
    }
    return true;
}

void SocketData::putMessage(std::string const& message)
{
    char const*     buffer      = &message[0];
    std::size_t     size        = message.size();
    std::size_t     dataWritten = 0;

    while(dataWritten < size)
    {
        std::size_t put = write(socketId, buffer + dataWritten, size - dataWritten);
        if (put == -1)
        {
            switch(errno)
            {
                case EINVAL:
                case EBADF:
                case ECONNRESET:
                case ENXIO:
                case EPIPE:
                {
                    // Fatal error. Programming bug
                    throw std::domain_error(buildErrorMessage("SocketData::putMessage: write: critical error: ", strerror(errno)));
                }
                case EDQUOT:
                case EFBIG:
                case EIO:
                case ENETDOWN:
                case ENETUNREACH:
                case ENOSPC:
                {
                    // Resource acquisition failure or device error
                    throw std::runtime_error(buildErrorMessage("SocketData::putMessage: write: resource failure: ", strerror(errno)));
                }
                case EAGAIN:
                case EINTR:
                {
                    // Temporrary error.
                    // Simply retry the read.
                    continue;
                }
                default:
                {
                    throw std::runtime_error(buildErrorMessage("SocketData::putMessage: write: returned -1: ", strerror(errno)));
                }
            }
        }
        dataWritten += put;
    }
    ::shutdown(socketId, SHUT_WR);
    return;
}

