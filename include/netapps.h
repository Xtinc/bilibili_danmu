#ifndef BEAST_NETAPPS_H
#define BEAST_NETAPPS_H

#include "templates.h"

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/beast/websocket.hpp>
#include <iostream>

namespace danmu
{
    namespace netbase = boost::asio;
    namespace netapps = boost::beast;

    inline static const char *USER_AGENT{"Mozilla/5.0 (Windows NT 10.0; Win64; x64) \
    AppleWebKit/537.36 (KHTML, like Gecko) Chrome/96.0.4664.110 Safari/537.36 Edg/96.0.1054.57"};
    inline static const char PING_PACK[31]{0, 0, 0, 0x1f, 0, 0x10, 0, 0x1, 0, 0, 0, 0x2, 0, 0, 0, 0x1,
                                           '[', 'o', 'b', 'j', 'e', 'c', 't', ' ', 'O', 'b', 'j', 'e', 'c', 't', ']'};
    struct STS_INFO
    {
        unsigned msg_c = 0;
        unsigned sc_c = 0;
        unsigned sc_p = 0;
        unsigned gf_p = 0;
        double gf_a = 0;
    };
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
    using DANMU_HANDLE = std::function<void(DANMU_MSG &)>;

    template <typename C, typename... CP>
    std::string findvalue(const std::string &str, C fch, CP... chs)
    {
        size_t strbegin = 0;
        auto strend = details::findvalue_t(strbegin, str, fch, chs...);
        return str.substr(strbegin, strend - strbegin);
    }
    template <typename T>
    netapps::http::response<T> sync_https_get(netbase::io_context &ioc, netbase::ssl::context &ctx,
                                              AUTHR_MSG &msg, int version)
    {
        netbase::ip::tcp::resolver resolver(ioc);
        netapps::ssl_stream<netapps::tcp_stream> stream(ioc, ctx);
        if (!SSL_set_tlsext_host_name(stream.native_handle(), msg.host.c_str()))
        {
            netapps::error_code ec(static_cast<int>(::ERR_get_error()), netbase::error::get_ssl_category());
            throw netapps::system_error{ec};
        }
        auto const results = resolver.resolve(msg.host, msg.port);
        netapps::get_lowest_layer(stream).connect(results);
        stream.handshake(netbase::ssl::stream_base::client);
        netapps::http::request<netapps::http::string_body> req{netapps::http::verb::get, msg.target, version};
        req.set(netapps::http::field::host, msg.host);
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
    inline const std::string currentDateTime(const char *fmt = "%Y-%m-%d.%X")
    {
        time_t now = time(0);
        struct tm tstruct;
        char buf[80];
        tstruct = *localtime(&now);
        // Visit http://en.cppreference.com/w/cpp/chrono/c/strftime
        // for more information about date/time format
        strftime(buf, sizeof(buf), fmt, &tstruct);
        return buf;
    }
    void co_wss_connect(AUTHR_MSG auth_msg, netbase::io_context &ioc,
                        netbase::ssl::context &ctx, netbase::yield_context yield);
    std::string auth_pack(unsigned int room_id, const char *key);
    void PrintBiliMsg(DANMU_MSG &info, STS_INFO &sts);

    class Parser
    {
    private:
        std::string remained;
        DANMU_HANDLE uhandler;

    public:
        Parser(DANMU_HANDLE &&uhd);
        ~Parser();
        Parser(const Parser &) = delete;
        Parser(Parser &&) = delete;
        Parser operator=(const Parser &) = delete;
        Parser operator=(Parser &&) = delete;

        void parser(const std::string &msg, size_t bytf);

    private:
        void process_data(const char *buf, unsigned int ilen, unsigned int typ);
    };

    class co_websocket : public std::enable_shared_from_this<co_websocket>
    {
    private:
        netapps::websocket::stream<netapps::ssl_stream<netapps::tcp_stream>> ws;
        netbase::steady_timer hbt;
        netbase::ip::tcp::resolver resolver;
        AUTHR_MSG auth_msg;
        STS_INFO sts;

    public:
        co_websocket(AUTHR_MSG _auth_msg, netbase::io_context &ioc, netbase::ssl::context &ctx);
        ~co_websocket();
        co_websocket(const co_websocket &) = delete;
        co_websocket(co_websocket &&) = delete;
        co_websocket operator=(const co_websocket &) = delete;
        co_websocket operator=(co_websocket &&) = delete;
        void co_connect(netbase::yield_context yield);
        void stop();
    };
}
#endif