
#include "ProtocolHTTP.h"
#include "Socket.h"
#include "Utility.h"
#include <iomanip>
#include <exception>
#include <algorithm>
#include <iostream>

/*
 * If it is not reading the body it buffers the data internally.
 *
 * Status/Header Lines:
 * ====================
 * When reading the status or headers it will return a single line at a time from
 *      getMessageData()
 *
 * It will prefer to use the internal buffer only reading from the socket when
 * required.
 * 
 * Body:
 * ====================
 * Will read data directly into the user provided buffer. If part of the body
 * is in the internal buffer it will be first copied to the user provided buffer
 * before a call to the socket is made for more data.
 *
 * Note:
 * ====================
 * This class assumes the socket connection will be reused as a result it will
 * maintain the input buffer between requests in case part of the next message
 * has been read.
 * BUT: Currently the sendMessage() for both client and server
 *      will close the socket with the call to socket.putMessageClose()
 * 
 */

using namespace ThorsAnvil::Socket;


ProtocolHTTP::ProtocolHTTP(DataSocket& socket)
    : Protocol(socket)
    , bufferData(bufferSize)
    , bufferRange(bufferData)
{}

/*
 * The functions to send a message using the HTTP Protocol
 *      sendMessage
 *          putMessageData
 *              socket
 */
void HTTPClient::sendMessage(std::string const& url, std::string const& message)
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
            throw std::logic_error(buildStringFromParts("ProtocolHTTP::", __func__, ": unsupported message type requested"));
    }


    // The Message Headers
    putMessageData("Content-Type: text/text\r\n");
    putMessageData(buildStringFromParts("Content-Length: ", message.size(), "\r\n"));
    putMessageData(buildStringFromParts("Host: ", getHost(), "\r\n"));
    putMessageData("User-Agent: ThorsExperimental-Client/0.1\r\n");
    putMessageData("Accept: */*\r\n");
    putMessageData("\r\n");

    // The Message Body
    putMessageData(message);
    socket.putMessageClose();
}

/*
 * Just Read the status line.
 * Validate it has the correct format and retrieve the status code.
 * As this may affect the size of the body.
 */
int HTTPClient::getMessageStartLine()
{
    if (getMessageData(nullptr, 0) == 0)
    {
        throw DropDisconnectedPipe("Read Failed for start line");
    }

    char    space1       = '\0';
    char    space2       = '\0';
    char    backslashR   = '\0';
    char    backslashN   = '\0';
    int     responseCode = 0;
    char    responseDescription[1024];
    int     count = std::sscanf(begin(), "HTTP/1.1%c%d%c%1023[^\r\n]%c%c",
                                &space1,
                                &responseCode,
                                &space2,
                                responseDescription,
                                &backslashR,
                                &backslashN);
    if (count != 6 || space1 != ' ' || space2 != ' ' || backslashR != '\r' || backslashN != '\n' || responseCode < 100 || responseCode >= 600)
    {
        throw std::runtime_error(buildErrorMessage("ProtocolHTTP::", __func__, ": Invalid HTTP Status Line:",
                                 " count(6)=", count,
                                 " space1(32)=", static_cast<int>(space1),
                                 " space2(32)=", static_cast<int>(space2),
                                 " backslashR(10)=", static_cast<int>(backslashR),
                                 " backslashN(13)=", static_cast<int>(backslashN),
                                 " responseCode=", responseCode,
                                 "Line: >", std::string(begin(), end()), "<"));
    }
    return responseCode;
}

/*
 * The functions to send a message using the HTTP Protocol
 *      sendMessage
 *          putMessageData
 *              socket
 */
void HTTPServer::sendMessage(std::string const&, std::string const& message)
{
    putMessageData("HTTP/1.1 200 OK\r\n");

    std::time_t t = std::time(nullptr);
    std::tm tm = *std::localtime(&t);

    // The Message Headers
    putMessageData(buildStringFromParts("Date: ", std::put_time(&tm, "%c %Z"), "\r\n"));
    putMessageData("Server: ThorsExperimental-Server/0.1\r\n");
    putMessageData(buildStringFromParts("Content-Length: ", message.size(), "\r\n"));
    putMessageData("Content-Type: text/text\r\n");
    putMessageData("\r\n");

    // The Message Body
    putMessageData(message);
    socket.putMessageClose();
}

int HTTPServer::getMessageStartLine()
{
    if (getMessageData(nullptr, 0) == 0)
    {
        throw DropDisconnectedPipe("Read Failed for start line");
    }



    char    command[32];
    char    url[4096];
    char    version[32];
    char    space1;
    char    space2;
    char    backslashR;
    char    backslashN;
    int     count = std::sscanf(begin(), "%s%c%s%c%s%c%c",
                                command,
                                &space1,
                                url,
                                &space2,
                                version,
                                &backslashR,
                                &backslashN);
    if (count != 7 || space1 != ' ' || space2 != ' ' || backslashR != '\r' || backslashN != '\n' || strncmp(version, "HTTP/1.", 7) != 0)
    {
        throw std::runtime_error(buildErrorMessage("ProtocolHTTP::", __func__, ": Invalid HTTP Request Line:",
                                 " count(7)=", count,
                                 " space1(32)=", static_cast<int>(space1),
                                 " space2(32)=", static_cast<int>(space2),
                                 " backslashR(10)=", static_cast<int>(backslashR),
                                 " backslashN(13)=", static_cast<int>(backslashN),
                                 " version(HTTP/1.1)=", version,
                                 " Line: >", std::string(begin(), end()), "<"));
    }
    return 200;
}

void ProtocolHTTP::putMessageData(std::string const& item)
{
    socket.putMessageData(item.c_str(), item.size());
}

/*
 * The functions to get a message using the HTTP Protocol
 *      recvMessage
 *          getMessageStartLine
 *          getMessageHeader
 *          getMessageBody
 *
 *      getMessageData
 *          getMessageDataFromBuffer
 *          getMessageDataFromStream
 *              socket
 */
void ProtocolHTTP::recvMessage(std::string& message)
{
    //std::cerr << "recvMessage 1\n";
    int         responseCode = getMessageStartLine();
    //std::cerr << "recvMessage 2\n";
    std::size_t bodySize     = getMessageHeader(responseCode);
    //std::cerr << "recvMessage 3\n";
    getMessageBody(bodySize, message);
    //std::cerr << "recvMessage 4\n";
}

/*
 * Read the headers for the stream.
 * Read each header in a loop (looking for the '\r\n' sequence.
 *
 * Do some validation on the input and calculate the size
 * of the message body based on the headers.
 */
std::size_t ProtocolHTTP::getMessageHeader(int responseCode)
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
            throw std::runtime_error(buildStringFromParts("ProtocolHTTP::", __func__, ": Header line not terminated by empty line"));
        }
        if (std::find(begOfRange, endOfRange, ':') == endOfRange)
        {
            throw std::runtime_error(buildStringFromParts("ProtocolHTTP::", __func__, ": Header line missing colon(:)"));
        }
        if (std::sscanf(begOfRange, "Transfer-Encoding : identity%c%c", &backslashR, &backslashN) == 2
            && backslashR == '\r' && backslashN == '\n')
        {
            hasIdentity         = true;
        }
        if (std::sscanf(begOfRange, "Content-Length : %lu%c%c", &contentLength, &backslashR, &backslashN) == 3
            && backslashR == '\r' && backslashN == '\n')
        {
            //std::cerr << "Has Length: " << contentLength << "\n";
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
        throw std::runtime_error(buildStringFromParts("ProtocolHTTP::", __func__, ": Header list not terminated by empty line"));
    }

    // Use the header fields to work out the size of the body/
    std::size_t bodySize = 0;
    if (responseCode < 200 || responseCode == 204 || responseCode == 304 || getRequestType() == Head)
    {
        bodySize = 0;
    }
    else if (hasIdentity)
    {
        throw std::domain_error(buildStringFromParts("ProtocolHTTP::", __func__, ": Identity encoding not supported"));
    }
    else if (hasContentLength)
    {
        bodySize = contentLength;
    }
    else if (hasMultiPart)
    {
        throw std::domain_error(buildStringFromParts("ProtocolHTTP::", __func__, ": Mult-Part encoding not supported"));
    }
    else
    {
        bodySize = -1;
    }
    //std::cerr << "Need: " << bodySize << "\n";
    return bodySize;
}

/*
 * If we have a `bodySize` of -1 then we read until the stream is closed.
 * Otherwise we read `bodySize` bytes from the stream.
 * 
 * Note: A closed connection by the client will stop the read and not generate
 *       any errors, but the string will be resize to the amount of data actually
 *       read from the stream.
 */
void ProtocolHTTP::getMessageBody(std::size_t bodySize, std::string& message)
{
    // The Message Body
    std::size_t maxBodySize = bodySize == static_cast<std::size_t>(-1) ? message.capacity() : bodySize;
    std::size_t messageRead = 0;
    std::size_t readSize;

    //std::cerr << "Get Body: " << bodySize << "\n";
    //std::cerr << "Get Max:  " << maxBodySize << "\n";
    // Allow us to use all the capacity of the string.
    message.resize(maxBodySize);
    while((readSize = getMessageData(&message[messageRead], maxBodySize - messageRead)) != 0)
    {
        messageRead += readSize;

        // If we have reached the capacity
        // The resize the string to allow for more data.
        if (messageRead == maxBodySize && bodySize == static_cast<std::size_t>(-1))
        {
            maxBodySize = maxBodySize * 1.5 + 10;
            message.resize(maxBodySize);
        }
    }
    // reset the size to the actual amount read.
    message.resize(messageRead);
}

/*
 * Read Data:
 *  Check to see if there is data in the local buffer and use that.
 *  Otherwise read from the socket.
 *
 * Note:
 * ========
 * If we are reading Status/Header information then `localBuffer`
 * will be set to nullptr and we should read the data into the buffer
 * for manual processing.
 *
 * If we are reading the Body the `localBuffer` points at the buffer
 * passed by the user so we can fill it with the content that is
 * coming from the stream.
 */
std::size_t ProtocolHTTP::getMessageData(char* localBuffer, std::size_t size)
{
    //std::cerr << "getMessageData\n";
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

std::size_t ProtocolHTTP::getMessageDataFromBuffer(char* localBuffer, std::size_t size)
{
    //std::cerr << "\tgetMeesageDataFromBuffer\n";
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

std::size_t ProtocolHTTP::getMessageDataFromStream(char* localBuffer, std::size_t size)
{
    //std::cerr << "\tgetMeesageDataFromStream(" << ((void*)localBuffer) << ", " << size << ")\n";
    char*           buffer    = localBuffer ? localBuffer : bufferRange.inputStart;
    std::size_t     dataRead  = localBuffer ? 0           : bufferRange.totalLength;
    std::size_t     dataMax   = localBuffer ? size        : bufferSize - (bufferRange.inputStart - &bufferData[0]);
    char*           lastCheck = buffer + (dataRead ? dataRead - 1 : 0);
    BufferRange&    br        = bufferRange;

    return socket.getMessageData(buffer + dataRead, dataMax, [localBuffer, &br, buffer, &lastCheck, dataRead](std::size_t readSoFar)
    {
        // Reading the Body.
        // There is no reason to stop just read as much as possible.
        if (localBuffer != nullptr)
        {
            return false;
        }

        // Reading the status line or one of the headers.
        // So once we have a line in the buffer stop reading and processes it.
        auto totalDataRead = dataRead + readSoFar;
        auto find = std::search(lastCheck, buffer + totalDataRead, endOfLineSeq, endOfLineSeq + 2);
        if (find != buffer + totalDataRead)
        {
            br.inputLength = find + 2 - buffer;
            br.totalLength += readSoFar;
            return true;
        }
        lastCheck = find - 1;
        return false;
    });
}

