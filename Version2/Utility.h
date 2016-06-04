
#ifndef THORSANVIL_SOCKET_UTILITY_H
#define THORSANVIL_SOCKET_UTILITY_H

#include <string>
#include <vector>
#include <sstream>

namespace ThorsAnvil
{
    namespace Socket
    {

template<std::size_t I = 0, typename... Tp>
inline typename std::enable_if<I == sizeof...(Tp), void>::type
print(std::ostream&, std::tuple<Tp...> const&)
{ }

template<std::size_t I = 0, typename... Tp>
inline typename std::enable_if<I < sizeof...(Tp), void>::type
print(std::ostream& s, std::tuple<Tp...> const& t)
{
    s << std::get<I>(t);
    print<I + 1, Tp...>(s, t);
}

template<typename... Args>
std::string buildStringFromParts(Args const&... args)
{
    std::stringstream msg;
    print(msg, std::make_tuple(args...));
    return msg.str();
}

template<typename... Args>
std::string buildErrorMessage(Args const&... args)
{
    return buildStringFromParts(args...);
}

    }
}

#endif

