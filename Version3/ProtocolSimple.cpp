
#include "ProtocolSimple.h"
#include "Socket.h"

using namespace ThorsAnvil::Socket;

void SimpleProtocol::sendMessage(std::string const& url, std::string const& message)
{
    socket.putMessageData(url.c_str(), url.size());
    socket.putMessageData(message.c_str(), message.size());
    socket.putMessageClose();
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

void SimpleProtocol::recvMessage(std::string& message)
{
    std::size_t     dataRead = 0;

    while(true)
    {
        // This outer loop handles resizing of the message when we run of space in the string.
        StringSizer        stringSizer(message, dataRead);
        std::size_t const  capacity = message.capacity();
        std::size_t const  dataMax  = capacity - 1;
        char*              buffer   = &message[0];

        std::size_t got = socket.getMessageData(buffer, dataMax, [](std::size_t){return false;});
        dataRead    += got;
        if (got == 0)
        {
            break;
        }

        // Resize the string buffer
        // So that next time around we can read more data.
        message.reserve(message.capacity() * 1.5);
    }
}

