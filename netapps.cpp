#include "include/netapps.h"
#include "include/Countdanmu.h"
#include <boost/asio/buffers_iterator.hpp>
#include <fstream>
#include <sqlite3.h>

using namespace danmu;
using namespace netapps;

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

danmu::co_websocket::co_websocket(AUTHR_MSG _auth_msg, STS_INFO &_sts, netbase::io_context &ioc, netbase::ssl::context &ctx)
    : ws(ioc, ctx), sts(_sts), hbt(ioc), resolver(ioc), auth_msg(_auth_msg)
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
    SQLPTR dbc;
    auto db_guard = MakeGuard([&dbc]
                              { sqlite3_close(dbc); });
    init_db(dbc, auth_msg.rid);
    netbase::spawn(yield,
                   [self, &dbc](netbase::yield_context yield)
                   {
                       std::ofstream file(std::to_string(self->auth_msg.rid) + ".txt", std::ios::app);
                       if (!file)
                       {
                           throw std::logic_error("Can't open output file for record.");
                       }
                       int sc_perid = 0;
                       int scp_perid = 0;
                       double msg_perid = 0;
                       double gf_perid = 0;
                       double gfp_perid = 0;

                       file << std::setw(20) << "#time" << std::setw(15) << "MSG_counts"
                            << std::setw(15) << "MSG/s" << std::setw(15) << "SC_counts"
                            << std::setw(15) << "SC/30s" << std::setw(15) << "SC_income"
                            << std::setw(15) << "SCI/30s" << std::setw(15) << "Gift_Wt"
                            << std::setw(15) << "GFWt/s" << std::setw(15) << "Gift_income"
                            << std::setw(15) << "GFI/s" << std::endl;
                       for (;;)
                       {
                           sqlite3_exec(dbc, "BEGIN", NULL, NULL, NULL);
                           msg_perid = self->sts.msg_c;
                           sc_perid = self->sts.sc_c;
                           scp_perid = self->sts.sc_p;
                           gf_perid = self->sts.gf_a;
                           gfp_perid = self->sts.gf_p;
                           self->hbt.expires_from_now(std::chrono::seconds(30));
                           self->hbt.async_wait(yield);
                           self->ws.async_write(netbase::buffer(PING_PACK, 31), yield);
                           file << std::setw(20) << currentDateTime()
                                << std::setw(15) << self->sts.msg_c << std::setw(15) << (self->sts.msg_c - msg_perid) / 30.0
                                << std::setw(15) << self->sts.sc_c << std::setw(15) << self->sts.sc_c - sc_perid
                                << std::setw(15) << self->sts.sc_p << std::setw(15) << self->sts.sc_p - scp_perid
                                << std::setw(15) << self->sts.gf_a << std::setw(15) << (self->sts.gf_a - gf_perid) / 30.0
                                << std::setw(15) << self->sts.gf_p / 1000.0 << std::setw(15) << (self->sts.gf_p - gfp_perid) / 30000.0
                                << std::endl;
                           sqlite3_exec(dbc, "COMMIT", NULL, NULL, NULL);
                       }
                   });
    flat_buffer buf;
    Parser pack_parser(std::bind(&PrintBiliMsg, std::placeholders::_1, std::ref(sts), std::ref(dbc)));
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
    ws.async_close(websocket::close_code::normal,
                   [](error_code ec)
                   {
                       if (ec && ec != netbase::error::operation_aborted)
                       {
                           throw system_error{ec};
                       }
                   });
}

void danmu::init_db(SQLPTR &db, unsigned int rid)
{
    int ec = sqlite3_open((std::to_string(rid) + ".sqlite").c_str(), &db);
    if (ec != SQLITE_OK)
    {
        throw std::logic_error("database initialize failed.");
    }
    const char *text = "create table danmu(ID INTEGER PRIMARY KEY AUTOINCREMENT,NAME TEXT NOT NULL,\
      MSG TEXT ,PRICE INT DEFAULT 0, TimeStamp NOT NULL DEFAULT (datetime(\'now\',\'localtime\')))";
    char *zErrmsg = nullptr;
    ec = sqlite3_exec(db, text, NULL, NULL, &zErrmsg);
}