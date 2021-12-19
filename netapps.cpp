#include "include/netapps.h"
#include "include/Countdanmu.h"
#include "include/Countdanmu.h"
#include <boost/asio/buffers_iterator.hpp>
#include <fstream>
#include <iomanip>
#include <ctime>
#include <zlib.h>

using namespace danmu;
using namespace netapps;

void danmu::co_wss_connect(AUTHR_MSG auth_msg, netbase::io_context &ioc,
                           netbase::ssl::context &ctx, netbase::yield_context yield)
{

    auto ws = std::make_shared<websocket::stream<ssl_stream<tcp_stream>>>(ioc, ctx);
    auto hbt = std::make_shared<netbase::steady_timer>(ioc);
    auto sts = std::make_shared<STS_INFO>();

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
                   [ws, hbt, sts, auth_msg](netbase::yield_context yield)
                   {
                       std::ofstream file(std::to_string(auth_msg.rid) + ".txt");
                       if (!file)
                       {
                           throw std::logic_error("Can't open output file for record.");
                       }
                       double msg_perid = 0;
                       file << std::setw(20) << "#time" << std::setw(12) << "msg_counts"
                            << std::setw(12) << "msg_sec" << std::setw(12) << "SC_counts"
                            << std::setw(12) << "SC_prices" << std::setw(12) << "Gift_Wt"
                            << std::setw(12) << "Gift_prices" << std::endl;
                       for (;;)
                       {
                           msg_perid = sts->msg_c;
                           hbt->expires_from_now(std::chrono::seconds(30));
                           hbt->async_wait(yield);
                           ws->async_write(netbase::buffer(PING_PACK, 31), yield);
                           file << std::setw(20) << currentDateTime() << std::setw(12)
                                << sts->msg_c << std::setw(12) << (sts->msg_c - msg_perid) / 30.0
                                << std::setw(12) << sts->sc_c << std::setw(12)
                                << sts->sc_p << std::setw(12) << sts->gf_a << std::setw(12)
                                << sts->gf_p / 1000.0 << std::endl;
                       }
                   });
    /*     flat_buffer buf;
        Parser pack_parser(std::bind(&PrintBiliMsg, std::placeholders::_1, sts));
        for (;;)
        {
            size_t bytf = ws->async_read(buf, yield);
            pack_parser.parser(buffers_to_string(buf.data()), bytf);
            buf.clear();
        }
        hbt->cancel();
        ws->async_close(websocket::close_code::normal, yield); */
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

void danmu::PrintBiliMsg(DANMU_MSG &info, STS_INFO &sts)
{
    switch (info.type)
    {
    case 0x03:
    {
        break;
    }
    case 0x05:
    {
        ParseJSON(info.buff.get(), sts);
        break;
    }
    case 0x08:
    {
        std::cout << "Link start..." << std::endl;
        break;
    }
    default:
    {
        std::cout << "Unknown msg type: " << info.type << info.buff << std::endl;
        break;
    }
    }
}

danmu::Parser::Parser(DANMU_HANDLE &&uhd)
    : uhandler(std::forward<DANMU_HANDLE>(uhd))
{
}

danmu::Parser::~Parser() {}

void danmu::Parser::parser(const std::string &msg, size_t len)
{
    size_t pos = 0;
    size_t ireclen;
    // currently bilibili always send complete packs in a frame.
    remained = msg;
    while (pos < len)
    {
        const unsigned char *precv = (const unsigned char *)(msg.c_str());
        if (len < pos + 16)
        {
            if (pos != 0)
            {
                remained = remained.substr(pos, len - pos);
            }
            return;
        }
        ireclen = precv[pos + 1] << 16 | precv[pos + 2] << 8 | precv[pos + 3];
        if (ireclen < 16 || ireclen > 5000)
        {
            // error
            remained.clear();
            return;
        }
        if (len < pos + ireclen)
        {
            if (pos != 0)
            {
                remained = remained.substr(pos, len - pos);
            }
            return;
        }
        unsigned int typ = check_pack(precv + pos);
        if (!typ)
        {
            // error
            remained.clear();
            return;
        }
        else
        {
            process_data(remained.c_str() + pos, static_cast<unsigned int>(ireclen), typ);
        }
        pos += ireclen;
    }
}

void danmu::Parser::process_data(const char *buff, unsigned int ilen, unsigned int typ)
{
    // version==2,compressed data.
    if (buff[7] != 2)
    {
        DANMU_MSG info;
        info.type = typ;
        info.ver = buff[7];
        info.len = ilen - 16;
        if (ilen > 16)
        {
            info.buff.reset(new char[info.len + 1]);
            memcpy(info.buff.get(), buff + 16, info.len);
            info.buff[info.len] = 0;
        }
        uhandler(info);
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
            std::cerr << "Zlib init failed." << std::endl;
            // throw std::logic_error("Zlib init failed.");
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
            info.buff[info.len] = 0;
            uhandler(info);
        } while (ret == Z_OK);
        if (!success)
        {
            std::cerr << "Decompression failed." << std::endl;
            return;
            // throw std::logic_error("Decompression failed.");
        }
        inflateEnd(&strm);
    }
    return;
};

danmu::co_websocket::co_websocket(AUTHR_MSG _auth_msg, netbase::io_context &ioc, netbase::ssl::context &ctx)
    : ws(ioc, ctx), hbt(ioc), resolver(ioc), auth_msg(_auth_msg)
{
}

danmu::co_websocket::~co_websocket() {}

void danmu::co_websocket::co_connect(netbase::yield_context yield)
{
    auto const results = resolver.async_resolve(auth_msg.host, auth_msg.port, yield);
    get_lowest_layer(ws).expires_after(std::chrono::seconds(15));
    auto ep = get_lowest_layer(ws).async_connect(results, yield);
    if (!SSL_set_tlsext_host_name(ws.next_layer().native_handle(), auth_msg.host.c_str()))
    {
        error_code ec(static_cast<int>(::ERR_get_error()), net::error::get_ssl_category());
        throw system_error{ec};
    }
    auth_msg.host += ':' + std::to_string(ep.port());
    get_lowest_layer(ws).expires_after(std::chrono::seconds(15));
    ws.set_option(websocket::stream_base::decorator(
        [](websocket::request_type &req)
        {
            req.set(http::field::user_agent, USER_AGENT);
        }));
    ws.next_layer().async_handshake(netbase::ssl::stream_base::client, yield);
    get_lowest_layer(ws).expires_never();
    ws.set_option(websocket::stream_base::timeout::suggested(role_type::client));
    ws.async_handshake(auth_msg.host, auth_msg.target, yield);
    ws.binary(true);
    auto authp = auth_pack(auth_msg.rid, auth_msg.key.c_str());
    ws.async_write(netbase::buffer(authp.data(), authp.size()), yield);
    ws.async_write(netbase::buffer(PING_PACK, 31), yield);
    auto self = shared_from_this();
    netbase::spawn(yield,
                   [self](netbase::yield_context yield)
                   {
                       std::ofstream file(std::to_string(self->auth_msg.rid) + ".txt");
                       if (!file)
                       {
                           throw std::logic_error("Can't open output file for record.");
                       }
                       double msg_perid = 0;
                       file << std::setw(20) << "#time" << std::setw(12) << "msg_counts"
                            << std::setw(12) << "msg_sec" << std::setw(12) << "SC_counts"
                            << std::setw(12) << "SC_prices" << std::setw(12) << "Gift_Wt"
                            << std::setw(12) << "Gift_prices" << std::endl;
                       for (;;)
                       {
                           msg_perid = self->sts.msg_c;
                           self->hbt.expires_from_now(std::chrono::seconds(30));
                           self->hbt.async_wait(yield);
                           self->ws.async_write(netbase::buffer(PING_PACK, 31), yield);
                           file << std::setw(20) << currentDateTime() << std::setw(12)
                                << self->sts.msg_c << std::setw(12) << (self->sts.msg_c - msg_perid) / 30.0
                                << std::setw(12) << self->sts.sc_c << std::setw(12)
                                << self->sts.sc_p << std::setw(12) << self->sts.gf_a << std::setw(12)
                                << self->sts.gf_p / 1000.0 << std::endl;
                       }
                   });
    flat_buffer buf;
    Parser pack_parser(std::bind(&PrintBiliMsg, std::placeholders::_1, std::ref(sts)));
    for (;;)
    {
        size_t bytf = ws.async_read(buf, yield);
        pack_parser.parser(buffers_to_string(buf.data()), bytf);
        buf.clear();
    }
}

void danmu::co_websocket::stop()
{
    hbt.cancel();
    ws.async_close(websocket::close_code::normal, [](error_code ec)
                   { std::cout << " Websocket closed." << std::endl; });
}