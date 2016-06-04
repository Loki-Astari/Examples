
#include "Protocol.h"
#include "Socket.h"
#include "Utility.h"

using namespace ThorsAnvil::Socket;

Protocol::Protocol(DataSocket& socket)
    : socket(socket)
{}

HTTPProtocol::HTTPProtocol(DataSocket& socket)
    : Protocol(socket)
    , bufferData(bufferSize)
    , bufferRange(bufferData)
{}

std::size_t HTTPProtocol::getMessageDataFromBuffer(char* localBuffer, std::size_t size)
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

std::size_t HTTPProtocol::getMessageData(char* localBuffer, std::size_t size)
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

int HTTPProtocol::getMessageStatus()
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
        throw std::runtime_error(buildErrorMessage("HTTPProtocol::getMessageStatus: Invalid HTTP Status Line:",
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

std::size_t HTTPProtocol::getMessageHeader(int responseCode)
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
            throw std::runtime_error("HTTPProtocol::getMessageHeader: Header line not terminated by empty line");
        }
        if (std::find(begOfRange, endOfRange, ':') == endOfRange)
        {
            throw std::runtime_error("HTTPProtocol::getMessageHeader: Header line missing colon(:)");
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
        throw std::runtime_error("HTTPProtocol::getMessageHeader: Header list not terminated by empty line");
    }

    // Use the header fields to work out the size of the body/
    std::size_t bodySize = 0;
    if (responseCode < 200 || responseCode == 204 || responseCode == 304 || getRequestType() == Head)
    {
        bodySize = 0;
    }
    else if (hasIdentity)
    {
        throw std::domain_error("HTTPProtocol::getMessageHeader: Identity encoding not supported");
    }
    else if (hasContentLength)
    {
        bodySize = contentLength;
    }
    else if (hasMultiPart)
    {
        throw std::domain_error("HTTPProtocol::getMessageHeader: Mult-Part encoding not supported");
    }
    else
    {
        bodySize = -1;
    }
    return bodySize;
}

void HTTPProtocol::getMessageBody(std::size_t bodySize, std::string& message)
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

void HTTPProtocol::recvMessage(std::string& message)
{
    int         responseCode = getMessageStatus();
    std::size_t bodySize     = getMessageHeader(responseCode);
    getMessageBody(bodySize, message);
}

std::size_t HTTPProtocol::getMessageDataFromStream(char* buffer, std::size_t size)
{
    return socket.getMessageData(buffer, size, [](std::size_t nextChunk)
    {
        return false;
    });
}
void HTTPProtocol::putMessageData(std::string const& item)
{
    socket.putMessageData(item.c_str(), item.size());
}

void HTTPProtocol::sendMessage(std::string const& url, std::string const& message)
{
    // The Message Method
    switch(getRequestType())
    {
        case Head:   putMessageData(buildStringFromParts("HEAD ",   url.c_str(), " HTTP/1.1\r\n"));   break;
        case Get:    putMessageData(buildStringFromParts("GET ",    url.c_str(), " HTTP/1.1\r\n"));   break;
        case Put:    putMessageData(buildStringFromParts("PUT ",    url.c_str(), " HTTP/1.1\r\n"));   break;
        case Post:   putMessageData(buildStringFromParts("POST ",   url.c_str(), " HTTP/1.1\r\n"));   break;
        case Delete: putMessageData(buildStringFromParts("DELETE ", url.c_str(), " HTTP/1.1\r\n"));   break;
        default:
            throw std::logic_error("HTTPProtocol::putMessage: unsupported message type requested");
    }


    // The Message Headers
    putMessageData("Content-Type: text/text\r\n");
    putMessageData(buildStringFromParts("Content-Length: ", message.size(), "\r\n"));
    putMessageData(buildStringFromParts("Host: ", getHost(), "\r\n"));
    putMessageData("User-Agent: ThorsExperimental/0.1\r\n");
    putMessageData("Accept: */*\r\n");
    putMessageData("\r\n");

    // The Message Body
    putMessageData(message);
    socket.putMessageClose();
}

