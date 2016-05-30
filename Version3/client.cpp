
#include <curl/curl.h>
#include <map>
#include <tuple>
#include <sstream>
#include <iostream>
#include <type_traits>
#include <cstdlib>

namespace ThorsAnvil
{
    namespace Socket
    {

template<std::size_t I = 0, typename... Tp>
inline typename std::enable_if<I == sizeof...(Tp), void>::type
print(std::ostream& s, std::tuple<Tp...> const& t)
{ }

template<std::size_t I = 0, typename... Tp>
inline typename std::enable_if<I < sizeof...(Tp), void>::type
print(std::ostream& s, std::tuple<Tp...> const& t)
{
    s << std::get<I>(t);
    print<I + 1, Tp...>(s, t);
}

template<typename... Args>
std::string buildErrorMessage(Args const&... args)
{
    std::stringstream msg;
    print(msg, std::make_tuple(args...));
    return msg.str();
}

class CurlGlobal
{
    public:
        CurlGlobal()
        {
            if (curl_global_init(CURL_GLOBAL_ALL) != 0)
            {
                throw std::runtime_error(buildErrorMessage("CurlGlobal::CurlGlobal: curl_global_init: fail"));
            }
        }
        ~CurlGlobal()
        {
            curl_global_cleanup();
        }
};

enum Method {Get, Head, Put, Post, Delete};
class CurlConnector
{
    CURL*       curl;
    public:
        CurlConnector(std::string const& domain, std::string const& path)
            : CurlConnector("https", domain, path, std::map<std::string, std::string>(), "")
        {}
        CurlConnector(std::string const& domain,
                      std::string const& path,
                      std::map<std::string, std::string> const& param,
                      std::string const& fragment = ""
                     )
            : CurlConnector("https", domain, path, param, fragment)
        {}
        CurlConnector(std::string const& schema,
                      std::string const& domain,
                      std::string const& path,
                      std::map<std::string, std::string> const& param,
                      std::string const& fragment = ""
                     )
            : curl(curl_easy_init( ))
        {
            if (curl == NULL)
            {
                throw std::runtime_error(buildErrorMessage("CurlConnector::CurlConnector: curl_easy_init: fail"));
            }
            CURLcode res;
            std::stringstream url;
            url << "http://" << domain << "/" << path;
            if (!param.empty())
            {
                std::string conector = "?";
                for(auto const& item: param)
                {
                    auto escapeDeleter = [](char* value){curl_free(value);};
                    std::unique_ptr<char, decltype(escapeDeleter)> value(curl_easy_escape(curl, item.second.c_str(), item.second.size()), escapeDeleter);
                    url << conector << item.first << "=" << value.get();
                    conector = "&";
                }
            }
            if (!fragment.empty())
            {
                url << "#" << fragment;
            }
            if ((res = curl_easy_setopt(curl, CURLOPT_URL, url.str().c_str())) != CURLE_OK)
            {
                throw std::runtime_error(buildErrorMessage("CurlConnector::CurlConnector: curl_easy_setopt CURLOPT_URL:", curl_easy_strerror(res)));
            }
        }
        ~CurlConnector()
        {
            curl_easy_cleanup(curl);
        }
        void perform(Method method, std::string const& data)
        {
            CURLcode res;
            if ((res = curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, data.size())) != CURLE_OK)
            {
                throw std::runtime_error(buildErrorMessage("CurlConnector::perform: curl_easy_setopt CURLOPT_POSTFIELDSIZE:", curl_easy_strerror(res)));
            }
            if ((res = curl_easy_setopt(curl, CURLOPT_COPYPOSTFIELDS, data.data())) != CURLE_OK)
            {
                throw std::runtime_error(buildErrorMessage("CurlConnector::perform: curl_easy_setopt CURLOPT_COPYPOSTFIELDS:", curl_easy_strerror(res)));
            }

            perform(method);
        }
        void perform(Method method)
        {
            CURLcode res;
            switch(method)
            {
                case Get:       res = CURLE_OK; /* The default is GET. So do nothing.*/         break;
                case Head:      res = curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "HEAD");    break;
                case Put:       res = curl_easy_setopt(curl, CURLOPT_PUT, 1);                   break;
                case Post:      res = curl_easy_setopt(curl, CURLOPT_POST, 1);                  break;
                case Delete:    res = curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");  break;
                default:
                    throw std::domain_error(buildErrorMessage("CurlConnector::perform: invalid method: ", static_cast<int>(method)));
            }
            if (res != CURLE_OK)
            {
                throw std::runtime_error(buildErrorMessage("CurlConnector::perform: curl_easy_setopt CURL_METHOD:", curl_easy_strerror(res)));
            }
            if ((res = curl_easy_perform(curl)) != CURLE_OK)
            {
                throw std::runtime_error(buildErrorMessage("CurlConnector::perform: curl_easy_perform:", curl_easy_strerror(res)));
            }
        }
};

    }
}

int main(int argc, char* argv[])
{
    namespace Sock = ThorsAnvil::Socket;
    if (argc != 3)
    {
        std::cerr << "Usage: client <host> <Message>\n";
        std::exit(1);
    }

    Sock::CurlGlobal    curlInit;
    std::map<std::string, std::string>  parameters {{"data", argv[2]}};
    Sock::CurlConnector connect("https", argv[1], "message", parameters);

    connect.perform(Sock::Get);
}

