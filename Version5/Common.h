
#ifndef THORSANVIL_SOCKET_VERSION5_COMMON_H
#define THORSANVIL_SOCKET_VERSION5_COMMON_H

#include <string>
#include <cstdlib>

namespace ThorsAnvil
{
    namespace Socket
    {

std::string commonSetUp(int argc, char* argv[])
{
    signal(SIGPIPE, SIG_IGN);

    std::string data    = "OK";
    if (argc > 1)
    {
        std::size_t size = std::atoi(argv[1]);
        data.resize(size);
        for(std::size_t loop = 0;loop < size; ++loop)
        {
            data[loop] = 'A' + (loop % 26);
        }
    }
    return data;
}

    }
}

#endif

