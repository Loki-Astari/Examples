#ifndef THORSANVIL_SOCKET_PROTOCOL_HTTP_H
#define THORSANVIL_SOCKET_PROTOCOL_HTTP_H

#include "Protocol.h"
#include <string>
#include <vector>
#include <cstddef>

namespace ThorsAnvil
{
    namespace Socket
    {

enum RequestType {Response, Head, Get, Put, Post, Delete};

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

    protected:
        char const*   begin()   const   {return bufferRange.inputStart;}
        char const*   end()     const   {return bufferRange.inputStart + bufferRange.inputLength;}

        virtual RequestType getRequestType() const = 0;

        void        putMessageData(std::string const& item);
        std::size_t getMessageData(char* localBuffer, std::size_t size);

        virtual int         getMessageStartLine() = 0;
        std::size_t getMessageHeader(int responseCode);
        void        getMessageBody(std::size_t bodySize, std::string& message);

        std::size_t getMessageDataFromStream(char* buffer, std::size_t size);
        std::size_t getMessageDataFromBuffer(char* localBuffer, std::size_t size);

    public:
        void recvMessage(std::string& message)                               override;
        ProtocolHTTP(DataSocket& socket);

};

class HTTPServer: public ProtocolHTTP
{
    private:
        int         getMessageStartLine() override;
        RequestType getRequestType() const override {return Response;}
    public:
        using ProtocolHTTP::ProtocolHTTP;
        void sendMessage(std::string const& url, std::string const& message) override;
};

class HTTPClient: public ProtocolHTTP
{
    private:
        int         getMessageStartLine() override;
        virtual std::string const& getHost() const = 0;
    public:
        using ProtocolHTTP::ProtocolHTTP;
        void sendMessage(std::string const& url, std::string const& message) override;
};

class HTTPPost: public HTTPClient
{
    std::string host;
    private:
        RequestType getRequestType() const override {return Post;}
        std::string const& getHost() const override {return host;}
    public:
        HTTPPost(std::string const& host, DataSocket& socket)
            : HTTPClient(socket)
            , host(host)
        {}
};


    }
}

#endif
