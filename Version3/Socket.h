
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

enum RequestType {Head, Get, Put, Post, Delete};

// A class that can read/write to a socket
class DataSocket: public BaseSocket
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
    std::size_t getMessageDataFromStream(char* localBuffer, std::size_t size);
    std::size_t getMessageData(char* localBuffer, std::size_t size);

    int         getMessageStatus();
    std::size_t getMessageHeader(RequestType requestType, int responseCode);
    void        getMessageBody(std::size_t bodySize, std::string& message);

    void putMessageData(std::string const& message);
    public:
        DataSocket(int socketId);

        DataSocket(DataSocket&& rhs)            noexcept;
        DataSocket& operator=(DataSocket&& rhs) noexcept;
        void swap(DataSocket& rhs)              noexcept;

        void getMessage(RequestType requestType, std::string& message);
        void putMessage(RequestType requestType, std::string const& host, std::string const& url, std::string const& message);
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

