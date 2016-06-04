
#include "Socket.h"
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
        throw std::runtime_error(buildErrorMessage("BaseSocket::BaseSocket: bad socket: ", strerror(errno)));
    }
}

BaseSocket::~BaseSocket()
{
    if (socketId == 0)
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
    if (socketId == 0)
    {
        throw std::logic_error(buildErrorMessage("DataSocket::close: accept called on a bad socket object (this object was moved)"));
    }
    while(true)
    {
        int state = ::close(socketId);
        if (state == 0)
        {
            break;
        }
        switch(errno)
        {
            case EBADF: throw std::domain_error(buildErrorMessage("BaseSocket::close: close: EBADF: ", socketId, " ", strerror(errno)));
            case EIO:   throw std::runtime_error(buildErrorMessage("BaseSocket::close: close: EIO:  ", socketId, " ", strerror(errno)));
            case EINTR:
            {
                        // TODO: Check for user interrupt flags.
                        //       Beyond the scope of this project
                        //       so continue normal operations.
                break;
            }
            default:    throw std::runtime_error(buildErrorMessage("BaseSocket::close: close: ???:  ", socketId, " ", strerror(errno)));
        }
    }
    socketId = 0;
}

void BaseSocket::swap(BaseSocket& other) noexcept
{
    using std::swap;
    swap(socketId,   other.socketId);
}

BaseSocket::BaseSocket(BaseSocket&& move) noexcept
    : socketId(0)
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
    struct sockaddr_in serverAddr;
    bzero((char*)&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family       = AF_INET;
    serverAddr.sin_port         = htons(port);
    serverAddr.sin_addr.s_addr  = inet_addr(host.c_str());

    if (::connect(getSocketId(), (struct sockaddr*)&serverAddr, sizeof(serverAddr)) != 0)
    {
        close();
        throw std::runtime_error(buildErrorMessage("ConnectSocket::ConnectSocket: connect: ", strerror(errno)));
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
        throw std::runtime_error(buildErrorMessage("ServerSocket::ServerSocket: bind: ", strerror(errno)));
    }

    if (::listen(getSocketId(), 5) != 0)
    {
        close();
        throw std::runtime_error(buildErrorMessage("ServerSocket::ServerSocket: listen: ", strerror(errno)));
    }
}

DataSocket ServerSocket::accept()
{
    if (getSocketId() == 0)
    {
        throw std::logic_error(buildErrorMessage("ServerSocket::accept: accept called on a bad socket object (this object was moved)"));
    }

    struct  sockaddr_storage    serverStorage;
    socklen_t                   addr_size   = sizeof serverStorage;
    int newSocket = ::accept(getSocketId(), (struct sockaddr*)&serverStorage, &addr_size);
    if (newSocket == -1)
    {
        throw std::runtime_error(buildErrorMessage("ServerSocket:accept: accept: ", strerror(errno)));
    }
    return DataSocket(newSocket);
}

// Utility class
// Used by getMessage() to open the string upto capacity size.
// Then on destruction resize to the actual size of the string.
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

DataSocket::DataSocket(int socketId)
    : BaseSocket(socketId)
    , bufferRange(bufferData)
{
    bufferData.resize(bufferSize);
}

DataSocket::DataSocket(DataSocket&& rhs) noexcept
    : BaseSocket(0)
    , bufferRange(bufferData)
{
    rhs.swap(*this);
}

DataSocket& DataSocket::operator=(DataSocket&& rhs) noexcept
{
    rhs.swap(*this);
    return *this;
}

void DataSocket::swap(DataSocket& rhs) noexcept
{
    static_cast<BaseSocket&>(rhs).swap(*this);

    using std::swap;
    swap(bufferData, rhs.bufferData);
    bufferRange.swap(rhs.bufferRange);
}

std::size_t DataSocket::getMessageDataFromBuffer(char* localBuffer, std::size_t size)
{
    bufferRange.inputStart  += bufferRange.inputLength;
    bufferRange.totalLength -= bufferRange.inputLength;
    bufferRange.inputLength = 0;

    std::size_t result     = 0;

    if (localBuffer != nullptr)
    {
        result      = std::min(bufferRange.totalLength, size);

        std::copy(bufferRange.inputStart, bufferRange.inputStart + result, localBuffer);
        bufferRange.totalLength -= result;
    }
    else
    {
        auto begOfRange = bufferRange.inputStart;
        auto endOfRange = bufferRange.inputStart + bufferRange.totalLength;
        auto find       = std::search(begOfRange, endOfRange, endOfLineSeq, endOfLineSeq + 2);
        if (find != endOfRange)
        {
            bufferRange.inputLength = find + 2 - bufferRange.inputStart;
            result  = bufferRange.inputLength;
        }
        else
        {
            // We found some of a header or the method in the buffer
            // But it was not the whole line. So move this fragment to
            // the beginning of the buffer and return 0 to indicate
            // that not a complete line was read. This will result in
            // a call to getMessageDataFromStream()
            std::copy(begOfRange, endOfRange, &bufferData[0]);
            bufferRange.inputStart = &bufferData[0];
        }
    }

    return result;
}

std::size_t DataSocket::getMessageDataFromStream(char* localBuffer, std::size_t size)
{
    char*           buffer    = localBuffer ? localBuffer : bufferRange.inputStart;
    std::size_t     dataRead  = localBuffer ? 0           : bufferRange.totalLength;
    std::size_t     dataMax   = localBuffer ? size        : bufferSize - (bufferRange.inputStart - &bufferData[0]);
    char*           lastCheck = buffer + (dataRead ? dataRead - 1 : 0);
    while(dataRead < dataMax)
    {
        // The inner loop handles interactions with the socket.
        std::size_t get = read(getSocketId(), buffer + dataRead, dataMax - dataRead);
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
                    throw std::domain_error(buildErrorMessage("DataSocket::getMessageDataFromStream: read: critical error: ", strerror(errno)));
                }
                case EIO:
                case ENOBUFS:
                case ENOMEM:
                {
                   // Resource acquisition failure or device error
                    throw std::runtime_error(buildErrorMessage("DataSocket::getMessageDataFromStream: read: resource failure: ", strerror(errno)));
                }
                case EINTR:
                    // TODO: Check for user interrupt flags.
                    //       Beyond the scope of this project
                    //       so continue normal operations.
                case ETIMEDOUT:
                case EAGAIN:
                {
                    // Temporary error.
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
                    throw std::runtime_error(buildErrorMessage("DataSocket::getMessageDataFromStream: read: returned -1: ", strerror(errno)));
                }
            }
        }
        if (get == 0)
        {
            break;
        }
        dataRead += get;
        if (localBuffer == nullptr)
        {
            auto find = std::search(lastCheck, buffer + dataRead, endOfLineSeq, endOfLineSeq + 2);
            if (find != buffer + dataRead)
            {
                bufferRange.inputLength = find + 2 - buffer;
                bufferRange.totalLength = dataRead;
                break;
            }
            lastCheck = find - 1;
        }
    }

    return dataRead;
}

std::size_t DataSocket::getMessageData(char* localBuffer, std::size_t size)
{

    if (bufferRange.totalLength != 0)
    {
        std::size_t result = getMessageDataFromBuffer(localBuffer, size);
        if (result != 0)
        {
            return result;
        }
    }
    else
    {
        bufferRange.inputStart  = &bufferData[0];
    }

    return getMessageDataFromStream(localBuffer, size);
}


int DataSocket::getMessageStatus()
{
    getMessageData(nullptr, 0);

    char    space1       = '\0';
    char    space2       = '\0';
    char    backslashR   = '\0';
    char    backslashN   = '\0';
    int     responseCode = 0;
    char    responseDescription[1024];
    int     count = std::sscanf(bufferRange.inputStart, "HTTP/1.1%c%d%c%1023[^\r\n]%c%c",
                                &space1,
                                &responseCode,
                                &space2,
                                responseDescription,
                                &backslashR,
                                &backslashN);
    if (count != 6 || space1 != ' ' || space2 != ' ' || backslashR != '\r' || backslashN != '\n' || responseCode < 100 || responseCode >= 600)
    {
        throw std::runtime_error(buildErrorMessage("DataSocket::getMessageStatus: Invalid HTTP Status Line:",
                                 "count(6): ", count,
                                 "space1(32): ", static_cast<int>(space1),
                                 "space2(32): ", static_cast<int>(space2),
                                 "backslashR(10): ", static_cast<int>(backslashR),
                                 "backslashN(13): ", static_cast<int>(backslashN),
                                 "responseCode: ", responseCode,
                                 "Line: ", std::string(bufferRange.inputStart, bufferRange.inputLength)));
    }
    return responseCode;
}

std::size_t DataSocket::getMessageHeader(RequestType requestType, int responseCode)
{
    char        backslashR       = '\0';
    char        backslashN       = '\0';
    bool        hasIdentity      = false;
    bool        hasContentLength = false;
    bool        hasMultiPart     = false;
    std::size_t contentLength = 0;

    char const* begOfRange = nullptr;
    char const* endOfRange = nullptr;
    while(getMessageData(nullptr, 0))
    {
        begOfRange = bufferRange.inputStart;
        endOfRange = bufferRange.inputStart + bufferRange.inputLength;
        if (bufferRange.inputLength <= 2)
        {
            break;
        }
        if (!std::equal(endOfRange - 2, endOfRange, endOfLineSeq))
        {
            throw std::runtime_error("DataSocket::getMessageHeader: Header line not terminated by empty line");
        }
        if (std::find(begOfRange, endOfRange, ':') == endOfRange)
        {
            throw std::runtime_error("DataSocket::getMessageHeader: Header line missing colon(:)");
        }
        if (std::sscanf(begOfRange, "Transfer-Encoding : identity%c%c", &backslashR, &backslashN) == 2
            && backslashR == '\r' && backslashN == '\n')
        {
            hasIdentity         = true;
        }
        if (std::sscanf(begOfRange, "Content-Length : %lu%c%c", &contentLength, &backslashR, &backslashN) == 3
            && backslashR == '\r' && backslashN == '\n')
        {
            hasContentLength    = true;
        }
        if (std::sscanf(begOfRange, "Content-Type : multipart/byteranges%c%c", &backslashR, &backslashN) == 3
            && backslashR == '\r' && backslashN == '\n')
        {
            hasMultiPart        = true;
        }
    }
    if (bufferRange.inputLength != 2 && !std::equal(begOfRange, endOfRange, endOfLineSeq))
    {
        throw std::runtime_error("DataSocket::getMessageHeader: Header list not terminated by empty line");
    }

    // Use the header fields to work out the size of the body/
    std::size_t bodySize = 0;
    if (responseCode < 200 || responseCode == 204 || responseCode == 304 || requestType == Head)
    {
        bodySize = 0;
    }
    else if (hasIdentity)
    {
        throw std::domain_error("DataSocket::getMessageHeader: Identity encoding not supported");
    }
    else if (hasContentLength)
    {
        bodySize = contentLength;
    }
    else if (hasMultiPart)
    {
        throw std::domain_error("DataSocket::getMessageHeader: Mult-Part encoding not supported");
    }
    else
    {
        bodySize = -1;
    }
    return bodySize;
}

void DataSocket::getMessageBody(std::size_t bodySize, std::string& message)
{
    // The Message Body
    std::size_t maxBodySize = bodySize == -1 ? message.capacity() : bodySize;
    std::size_t messageRead = 0;
    std::size_t readSize;
    if (bodySize != -1)
    {
        message.resize(maxBodySize);
    }
    while((readSize = getMessageData(&message[messageRead], maxBodySize - messageRead)) != 0)
    {
        messageRead += readSize;
        if (messageRead == maxBodySize && bodySize == -1)
        {
            maxBodySize = maxBodySize * 1.5 + 10;
            message.resize(maxBodySize);
        }
    }
    message.resize(messageRead);
}

void DataSocket::getMessage(RequestType requestType, std::string& message)
{
    if (getSocketId() == 0)
    {
        throw std::logic_error(buildErrorMessage("DataSocket::getMessage: accept called on a bad socket object (this object was moved)"));
    }

    int         responseCode = getMessageStatus();
    std::size_t bodySize     = getMessageHeader(requestType, responseCode);
    getMessageBody(bodySize, message);
}

void DataSocket::putMessageData(std::string const& message)
{
    char const*     buffer      = &message[0];
    std::size_t     size        = message.size();
    std::size_t     dataWritten = 0;

    while(dataWritten < size)
    {
        std::size_t put = write(getSocketId(), buffer + dataWritten, size - dataWritten);
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
                    throw std::domain_error(buildErrorMessage("DataSocket::putMessageData: write: critical error: ", strerror(errno)));
                }
                case EDQUOT:
                case EFBIG:
                case EIO:
                case ENETDOWN:
                case ENETUNREACH:
                case ENOSPC:
                {
                    // Resource acquisition failure or device error
                    throw std::runtime_error(buildErrorMessage("DataSocket::putMessageData: write: resource failure: ", strerror(errno)));
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
                    throw std::runtime_error(buildErrorMessage("DataSocket::putMessageData: write: returned -1: ", strerror(errno)));
                }
            }
        }
        dataWritten += put;
    }
    return;
}

void DataSocket::putMessage(RequestType requestType, std::string const& host, std::string const& url, std::string const& message)
{
    if (getSocketId() == 0)
    {
        throw std::logic_error(buildErrorMessage("DataSocket::putMessage: accept called on a bad socket object (this object was moved)"));
    }

    // The Message Method
    switch(requestType)
    {
        case Head:   putMessageData(buildStringFromParts("HEAD ",   url.c_str(), " HTTP/1.1\r\n"));   break;
        case Get:    putMessageData(buildStringFromParts("GET ",    url.c_str(), " HTTP/1.1\r\n"));   break;
        case Put:    putMessageData(buildStringFromParts("PUT ",    url.c_str(), " HTTP/1.1\r\n"));   break;
        case Post:   putMessageData(buildStringFromParts("POST ",   url.c_str(), " HTTP/1.1\r\n"));   break;
        case Delete: putMessageData(buildStringFromParts("DELETE ", url.c_str(), " HTTP/1.1\r\n"));   break;
        default:
            throw std::logic_error("DataSocket::putMessage: unsupported message type requested");
    }


    // The Message Headers
    putMessageData("Content-Type: text/text\r\n");
    putMessageData(buildStringFromParts("Content-Length: ", message.size(), "\r\n"));
    putMessageData(buildStringFromParts("Host: ", host, "\r\n"));
    putMessageData("User-Agent: ThorsExperimental/0.1\r\n");
    putMessageData("Accept: */*\r\n");
    putMessageData("\r\n");

    // The Message Body
    putMessageData(message);
    if (::shutdown(getSocketId(), SHUT_WR) != 0)
    {
        throw std::domain_error(buildErrorMessage("DataSocket::putMessage: shutdown: critical error: ", strerror(errno)));
    }
}


