#ifndef BEAST_NETAPPS_H
#define BEAST_NETAPPS_H

#include "templates.h"
#include "symbols.h"

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

    std::string auth_pack(unsigned int room_id, const char *key);

    class co_websocket : public std::enable_shared_from_this<co_websocket>
    {
    private:
        netapps::websocket::stream<netapps::ssl_stream<netapps::tcp_stream>> ws;
        netbase::steady_timer hbt;
        netbase::ip::tcp::resolver resolver;
        AUTHR_MSG auth_msg;
        STS_INFO &sts;

    public:
        co_websocket(AUTHR_MSG _auth_msg, STS_INFO &_sts, netbase::io_context &ioc, netbase::ssl::context &ctx);
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