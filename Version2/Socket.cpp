
#include "Socket.h"
#include "Utility.h"
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sstream>
#include <stdexcept>

#include <iostream>

using namespace ThorsAnvil::Socket;

BaseSocket::BaseSocket(int socketId)
    : socketId(socketId)
{
    if (socketId == -1)
    {
        throw std::runtime_error(buildErrorMessage("BaseSocket::", __func__, ": bad socket: ", strerror(errno)));
    }
}

BaseSocket::~BaseSocket()
{
    if (socketId == invalidSocketId)
    {
        // This object has been closed or moved.
        // So we don't need to call close.
        return;
    }

    try
    {
        close();
    }
    catch(...)
    {
        // We should log this 
        // TODO: LOGGING CODE HERE

        // If the user really want to catch close errors
        // they should call close() manually and handle
        // any generated exceptions. By using the
        // destructor they are indicating that failures is
        // an OK condition.
    }
}

void BaseSocket::close()
{
    if (socketId == invalidSocketId)
    {
        throw std::logic_error(buildErrorMessage("DataSocket::", __func__, ": accept called on a bad socket object (this object was moved)"));
    }
    while(true)
    {
        int state = ::close(socketId);
        if (state == invalidSocketId)
        {
            break;
        }
        switch(errno)
        {
            case EBADF: throw std::domain_error(buildErrorMessage("BaseSocket::", __func__, ": close: EBADF: ", socketId, " ", strerror(errno)));
            case EIO:   throw std::runtime_error(buildErrorMessage("BaseSocket::", __func__, ": close: EIO:  ", socketId, " ", strerror(errno)));
            case EINTR:
            {
                        // TODO: Check for user interrupt flags.
                        //       Beyond the scope of this project
                        //       so continue normal operations.
                break;
            }
            default:    throw std::runtime_error(buildErrorMessage("BaseSocket::", __func__, ": close: ???:  ", socketId, " ", strerror(errno)));
        }
    }
    socketId = invalidSocketId;
}

void BaseSocket::swap(BaseSocket& other) noexcept
{
    using std::swap;
    swap(socketId,   other.socketId);
}

BaseSocket::BaseSocket(BaseSocket&& move) noexcept
    : socketId(invalidSocketId)
{
    move.swap(*this);
}

BaseSocket& BaseSocket::operator=(BaseSocket&& move) noexcept
{
    move.swap(*this);
    return *this;
}

ConnectSocket::ConnectSocket(std::string const& host, int port)
    : DataSocket(::socket(PF_INET, SOCK_STREAM, 0))
{
    struct sockaddr_in serverAddr{};
    serverAddr.sin_family       = AF_INET;
    serverAddr.sin_port         = htons(port);
    serverAddr.sin_addr.s_addr  = inet_addr(host.c_str());

    if (::connect(getSocketId(), (struct sockaddr*)&serverAddr, sizeof(serverAddr)) != 0)
    {
        close();
        throw std::runtime_error(buildErrorMessage("ConnectSocket::", __func__, ": connect: ", strerror(errno)));
    }
}

ServerSocket::ServerSocket(int port)
    : BaseSocket(::socket(PF_INET, SOCK_STREAM, 0))
{
    struct sockaddr_in serverAddr;
    bzero((char*)&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family       = AF_INET;
    serverAddr.sin_port         = htons(port);
    serverAddr.sin_addr.s_addr  = INADDR_ANY;

    if (::bind(getSocketId(), (struct sockaddr *) &serverAddr, sizeof(serverAddr)) != 0)
    {
        close();
        throw std::runtime_error(buildErrorMessage("ServerSocket::", __func__, ": bind: ", strerror(errno)));
    }

    if (::listen(getSocketId(), maxConnectionBacklog) != 0)
    {
        close();
        throw std::runtime_error(buildErrorMessage("ServerSocket::", __func__, ": listen: ", strerror(errno)));
    }
}

DataSocket ServerSocket::accept()
{
    if (getSocketId() == invalidSocketId)
    {
        throw std::logic_error(buildErrorMessage("ServerSocket::", __func__, ": accept called on a bad socket object (this object was moved)"));
    }

    struct  sockaddr_storage    serverStorage;
    socklen_t                   addr_size   = sizeof serverStorage;
    int newSocket = ::accept(getSocketId(), (struct sockaddr*)&serverStorage, &addr_size);
    if (newSocket == -1)
    {
        throw std::runtime_error(buildErrorMessage("ServerSocket:", __func__, ": accept: ", strerror(errno)));
    }
    return DataSocket(newSocket);
}

void DataSocket::putMessageData(char const* buffer, std::size_t size)
{
    std::size_t     dataWritten = 0;

    while(dataWritten < size)
    {
        std::size_t put = write(getSocketId(), buffer + dataWritten, size - dataWritten);
        if (put == static_cast<std::size_t>(-1))
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
                    throw std::domain_error(buildErrorMessage("DataSocket::", __func__, ": write: critical error: ", strerror(errno)));
                }
                case EDQUOT:
                case EFBIG:
                case EIO:
                case ENETDOWN:
                case ENETUNREACH:
                case ENOSPC:
                {
                    // Resource acquisition failure or device error
                    throw std::runtime_error(buildErrorMessage("DataSocket::", __func__, ": write: resource failure: ", strerror(errno)));
                }
                case EINTR:
                        // TODO: Check for user interrupt flags.
                        //       Beyond the scope of this project
                        //       so continue normal operations.
                case EAGAIN:
                {
                    // Temporary error.
                    // Simply retry the read.
                    continue;
                }
                default:
                {
                    throw std::runtime_error(buildErrorMessage("DataSocket::", __func__, ": write: returned -1: ", strerror(errno)));
                }
            }
        }
        dataWritten += put;
    }
    return;
}

void DataSocket::putMessageClose()
{
    if (::shutdown(getSocketId(), SHUT_WR) != 0)
    {
        throw std::domain_error(buildErrorMessage("HTTPProtocol::", __func__, ": shutdown: critical error: ", strerror(errno)));
    }
}

