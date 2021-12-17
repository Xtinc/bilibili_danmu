#ifndef BEAST_NETAPPS_H
#define BEAST_NETAPPS_H

#include "templates.h"

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/beast/websocket.hpp>

namespace danmu
{
    namespace netbase = boost::asio;
    namespace netapps = boost::beast;

    inline static const char *USER_AGENT{"Mozilla/5.0 (Windows NT 10.0; Win64; x64) \
    AppleWebKit/537.36 (KHTML, like Gecko) Chrome/96.0.4664.110 Safari/537.36 Edg/96.0.1054.57"};
    inline static const char PING_PACK[31]{0, 0, 0, 0x1f, 0, 0x10, 0, 0x1, 0, 0, 0, 0x2, 0, 0, 0, 0x1,
                                           '[', 'o', 'b', 'j', 'e', 'c', 't', ' ', 'O', 'b', 'j', 'e', 'c', 't', ']'};
    struct AUTHR_MSG
    {
        std::string host;
        std::string port;
        std::string target;
        std::string key;
        unsigned int rid;
    };
    struct DANMU_MSG
    {
        unsigned type = 0;
        unsigned ver = 0;
        unsigned len = 0;
        std::unique_ptr<char[]> buff;
    };

    template <typename C, typename... CP>
    std::string findvalue(const std::string &str, C fch, CP... chs)
    {
        size_t strbegin = 0;
        auto strend = details::findvalue_t(strbegin, str, fch, chs...);
        return str.substr(strbegin, strend - strbegin);
    }
    template <typename T>
    netapps::http::response<T> sync_https_get(netbase::io_context &ioc, netbase::ssl::context &ctx,
                                              std::string &host, const std::string &port, const std::string &target, int version)
    {
        netbase::ip::tcp::resolver resolver(ioc);
        netapps::ssl_stream<netapps::tcp_stream> stream(ioc, ctx);
        if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str()))
        {
            netapps::error_code ec(static_cast<int>(::ERR_get_error()), netbase::error::get_ssl_category());
            throw netapps::system_error{ec};
        }
        auto const results = resolver.resolve(host, port);
        netapps::get_lowest_layer(stream).connect(results);
        stream.handshake(netbase::ssl::stream_base::client);
        netapps::http::request<netapps::http::string_body> req{netapps::http::verb::get, target, version};
        req.set(netapps::http::field::host, host);
        req.set(netapps::http::field::user_agent, USER_AGENT);
        netapps::http::write(stream, req);
        netapps::http::response<T> resp;
        netapps::http::read(stream, netapps::flat_buffer{}, resp);
        netapps::error_code ec;
        stream.shutdown(ec);
        if (ec && ec != netbase::error::eof && ec != netbase::ssl::error::stream_truncated)
        {
            throw netapps::system_error{ec};
        }
        return resp;
    }
    inline unsigned check_pack(const unsigned char *str)
    {
        int i;
        if (str[4])
        {
            return 0;
        }
        if (str[5] - 16)
        {
            return 0;
        }
        for (i = 8; i < 11; i++)
        {
            if (str[i])
                return 0;
        }
        for (i = 12; i < 15; i++)
        {
            if (str[i])
                return 0;
        }
        return str[11];
    }
    void co_wss_connect(AUTHR_MSG auth_msg, netbase::io_context &ioc,
                        netbase::ssl::context &ctx, netbase::yield_context yield);
    std::string auth_pack(unsigned int room_id, const char *key);
    std::string ping_pack();
    void process_message(std::string &msg, size_t len);
    void process_pack(const char *buff, const unsigned ilen, const unsigned type);
}
#endif