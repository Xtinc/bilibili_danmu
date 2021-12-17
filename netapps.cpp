#include "netapps.h"
#include <boost/asio/buffers_iterator.hpp>
#include <iostream>
#include <zlib.h>

using namespace danmu;
using namespace netapps;

void danmu::co_wss_connect(AUTHR_MSG auth_msg, netbase::io_context &ioc,
                           netbase::ssl::context &ctx, netbase::yield_context yield)
{

    auto ws = std::make_shared<websocket::stream<ssl_stream<tcp_stream>>>(ioc, ctx);
    auto hbt = std::make_shared<netbase::steady_timer>(ioc);

    netbase::ip::tcp::resolver resolver(ioc);
    auto const results = resolver.async_resolve(auth_msg.host, auth_msg.port, yield);
    get_lowest_layer(*ws).expires_after(std::chrono::seconds(15));
    auto ep = get_lowest_layer(*ws).async_connect(results, yield);
    if (!SSL_set_tlsext_host_name(ws->next_layer().native_handle(), auth_msg.host.c_str()))
    {
        error_code ec(static_cast<int>(::ERR_get_error()), net::error::get_ssl_category());
        throw system_error{ec};
    }
    auth_msg.host += ':' + std::to_string(ep.port());
    get_lowest_layer(*ws).expires_after(std::chrono::seconds(15));
    ws->set_option(websocket::stream_base::decorator(
        [](websocket::request_type &req)
        {
            req.set(http::field::user_agent, USER_AGENT);
        }));
    ws->next_layer().async_handshake(netbase::ssl::stream_base::client, yield);
    get_lowest_layer(*ws).expires_never();
    ws->set_option(websocket::stream_base::timeout::suggested(role_type::client));
    ws->async_handshake(auth_msg.host, auth_msg.target, yield);
    ws->binary(true);
    auto authp = auth_pack(auth_msg.rid, auth_msg.key.c_str());
    ws->async_write(netbase::buffer(authp.data(), authp.size()), yield);
    ws->async_write(netbase::buffer(PING_PACK, 31), yield);
    netbase::spawn(yield,
                   [ws, hbt](netbase::yield_context yield2)
                   {
                       for (;;)
                       {
                           hbt->expires_from_now(std::chrono::seconds(30));
                           hbt->async_wait(yield2);
                           ws->async_write(netbase::buffer(PING_PACK, 31), yield2);
                       }
                   });
    flat_buffer buf;
    for (;;)
    {
        size_t bytf = ws->async_read(buf, yield);
        process_message(buffers_to_string(buf.data()), bytf);
        buf.clear();
    }
    hbt->cancel();
    ws->async_close(websocket::close_code::normal, yield);
}

std::string danmu::auth_pack(unsigned int room_id, const char *key)
{
    char msg[528] = {};
    uint32_t buflen = sprintf(msg + 16,
                              "{\"uid\":0,\"roomid\":%d,\"protover\":%d,\"platform\":\"%s\",\"clientver\":\"%s\",\"type\":%d,\"key\":\"%s\"}",
                              room_id, 2, "web", "1.8.11", 2, key);
    if (buflen == -1)
    {
        throw std::logic_error("Invalid auth_pack arguments.");
    }
    uint32_t plen = htonl(buflen + 16);
    uint16_t hlen = htons(16);
    uint16_t pver = htons(1);
    uint32_t popc = htonl(7);
    uint32_t pseq = htonl(1);
    memcpy(&msg[0], &plen, 4);
    memcpy(&msg[4], &hlen, 2);
    memcpy(&msg[6], &pver, 2);
    memcpy(&msg[8], &popc, 4);
    memcpy(&msg[12], &pseq, 4);
    return std::string(msg, buflen + 16);
}

void danmu::process_message(std::string &msg, size_t len)
{
    size_t pos = 0;
    size_t ireclen;
    while (pos < len)
    {
        const unsigned char *precv = (const unsigned char *)msg.c_str();
        if (len < pos + 16)
        {
            return;
        }
        ireclen = precv[pos + 1] << 16 | precv[pos + 2] << 8 | precv[pos + 3];
        if (ireclen < 16 || ireclen > 5000)
        {
            return;
        }
        if (len < pos + ireclen)
        {
            return;
        }
        unsigned int type = check_pack(precv + pos);
        if (!type)
        {
            return;
        }
        process_pack(msg.c_str() + pos, (unsigned int)ireclen, type);
        pos += ireclen;
    }
}

void danmu::process_pack(const char *buff, const unsigned int ilen, const unsigned int type)
{
    if (buff[7] != 2)
    {
        DANMU_MSG info;
        info.type = type;
        info.ver = buff[7];
        info.len = ilen - 16;
        if (ilen > 16)
        {
            info.buff.reset(new char[info.len + 1]);
            memcpy(info.buff.get(), buff + 16, info.len);
            info.buff.get()[info.len] = 0;
        }
        // handler_msg(&info);
        std::cout << info.buff << std::endl;
        return;
    }
    else
    {
        z_stream strm;
        strm.zalloc = Z_NULL;
        strm.zfree = Z_NULL;
        strm.opaque = Z_NULL;
        strm.avail_in = 0;
        strm.next_in = Z_NULL;
        if (inflateInit(&strm) != Z_OK)
        {
            throw std::logic_error("Zlib init failed.");
            return;
        }
        strm.avail_in = ilen - 16;
        strm.next_in = (unsigned char *)buff + 16;
        int ret = 0;
        bool success = true;
        do
        {
            unsigned char head[17] = {};
            strm.avail_out = 16;
            strm.next_out = head;
            ret = inflate(&strm, Z_NO_FLUSH);
            if (ret != Z_OK && ret != Z_STREAM_END)
            {
                success = false;
                break;
            }
            if (strm.avail_out)
            {
                success = false;
                break;
            }
            DANMU_MSG info;
            info.type = check_pack(head);
            info.ver = head[7];
            info.len = head[0] << 24 | head[1] << 16 | head[2] << 8 | head[3];
            info.len -= 16;
            info.buff.reset(new char[info.len + 1]);
            if (!info.type)
            {
                success = false;
                break;
            }
            strm.avail_out = info.len;
            strm.next_out = (unsigned char *)info.buff.get();
            ret = inflate(&strm, Z_NO_FLUSH);
            if (ret != Z_OK && ret != Z_STREAM_END)
            {
                success = false;
                break;
            }
            if (strm.avail_out)
            {
                success = false;
                break;
            }
            info.buff.get()[info.len] = 0;
            // handler_msg(&info);
            std::cout << info.buff << std::endl;
        } while (ret == Z_OK);
        if (!success)
        {
            throw std::logic_error("Decompression failed.");
        }
        inflateEnd(&strm);
    }
    return;
}