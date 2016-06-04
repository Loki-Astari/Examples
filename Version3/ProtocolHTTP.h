
#ifndef THORSANVIL_SOCKET_PROTOCOL_HTTP_H
#define THORSANVIL_SOCKET_PROTOCOL_HTTP_H

#include "Protocol.h"
#include <vector>
#include <sstream>

namespace ThorsAnvil
{
    namespace Socket
    {

enum RequestType {Head, Get, Put, Post, Delete};

class ProtocolHTTP: public Protocol
{
    struct BufferRange
    {
        char*       inputStart;
        std::size_t inputLength;
        std::size_t totalLength;
        BufferRange(std::vector<char>& buffer)
            : inputStart(&buffer[0])
            , inputLength(0)
            , totalLength(0)
        {}
        void swap(BufferRange& rhs) noexcept
        {
            using std::swap;
            swap(inputStart,  rhs.inputStart);
            swap(inputLength, rhs.inputLength);
            swap(totalLength, rhs.totalLength);
        }
    };
    static constexpr char const* endOfLineSeq = "\r\n";
    static constexpr std::size_t bufferSize   = 4096;
    std::vector<char>           bufferData;
    BufferRange                 bufferRange;

    std::size_t getMessageDataFromBuffer(char* localBuffer, std::size_t size);
    std::size_t getMessageData(char* localBuffer, std::size_t size);

    int         getMessageStatus();
    std::size_t getMessageHeader(int responseCode);
    void        getMessageBody(std::size_t bodySize, std::string& message);

    std::size_t getMessageDataFromStream(char* buffer, std::size_t size);
    void putMessageData(std::string const& item);
    public:
        ProtocolHTTP(DataSocket& socket);

        void sendMessage(std::string const& url, std::string const& message) override;
        void recvMessage(std::string& message)                               override;
    private:
        virtual RequestType getRequestType() const = 0;
        virtual std::string const& getHost() const = 0;
};

class HTTPPost: public ProtocolHTTP
{
    std::string host;
    private:
        RequestType getRequestType() const override {return Post;}
        std::string const& getHost() const override {return host;}
    public:
        HTTPPost(std::string const& host, DataSocket& socket)
            : ProtocolHTTP(socket)
            , host(host)
        {}
};


    }
}

#endif

