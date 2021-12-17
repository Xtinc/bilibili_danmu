#include "netapps.h"
#include <iostream>
#include <boost/asio/signal_set.hpp>

using namespace danmu;
using namespace netapps;

void ParserCMD(std::atomic<bool> &closeflag)
{
    try
    {
        std::string str;
        while (std::cin >> str)
        {
            if (str == "quit")
            {
                closeflag.store(true);
                break;
            }
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Thread-Exception(thread" << std::this_thread::get_id() << "):" << e.what() << std::endl;
    }
    catch (...)
    {
        std::cerr << "Thread-Exception(thread" << std::this_thread::get_id() << ")" << std::endl;
    }
}

int main(int, char **)
{
    try
    {
        std::atomic<bool> closeflag(false);
        netbase::io_context ioc;
        netbase::ssl::context ctx{netbase::ssl::context::tlsv12};
        ctx.set_verify_mode(netbase::ssl::verify_none);
        AUTHR_MSG auth_msg;
        auth_msg.host = "api.live.bilibili.com";
        auth_msg.port = "443";
        auth_msg.target = "/room/v1/Room/room_init?id=270689";
        auto resp = sync_https_get<http::string_body>(ioc, ctx, auth_msg.host, auth_msg.port, auth_msg.target, 11).body();
        std::string rid = findvalue(resp, "data", "room_id\":", ",");
        auth_msg.rid = stoi(rid);
        auth_msg.target = "/room/v1/Danmu/getConf?room_id=" + rid + "&platform=pc&player=12176";
        resp = sync_https_get<http::string_body>(ioc, ctx, auth_msg.host, auth_msg.port, auth_msg.target, 11).body();
        auth_msg.key = findvalue(resp, "data", "token\":\"", "\"");
        auth_msg.host = "broadcastlv.chat.bilibili.com";
        auth_msg.target = "/sub";
        netbase::signal_set signals(ioc, SIGINT, SIGTERM);
        netbase::spawn(ioc, std::bind(&co_wss_connect, auth_msg, std::ref(ioc),
                                      std::ref(ctx), std::placeholders::_1));
        signals.async_wait(
            [&](error_code const &, int)
            {
                ioc.stop();
            });
        ioc.run();
    }
    catch (std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
