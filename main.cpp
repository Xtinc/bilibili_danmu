#include "include/netapps.h"
#include <iostream>
#include <boost/asio/signal_set.hpp>
#include <boost/program_options.hpp>

using namespace danmu;
using namespace netapps;
namespace bopt = boost::program_options;

int main(int argc, char **argv)
{
    try
    {
        bopt::options_description opts("danmu options");
        opts.add_options()("help", "just a help info")("room", bopt::value<std::string>(), "room id to be connected");
        bopt::variables_map vm;
        bopt::store(bopt::parse_command_line(argc, argv, opts), vm);
        std::string roomlist;
        if (vm.count("help"))
        {
            std::cout << opts << std::endl;
            return EXIT_SUCCESS;
        }
        if (vm.count("room"))
        {
            roomlist = vm["room"].as<std::string>();
        }
        else
        {
            std::cout << opts << std::endl;
            return EXIT_SUCCESS;
        }
        // cmd opt parser
        std::ios_base::sync_with_stdio(false);
        netbase::io_context ioc;
        netbase::ssl::context ctx{netbase::ssl::context::tlsv12};
        ctx.set_verify_mode(netbase::ssl::verify_none);
        // auto ws = std::make_shared<websocket::stream<ssl_stream<tcp_stream>>>(ioc, ctx);
        // auto hbt = std::make_shared<netbase::steady_timer>(ioc);
        //
        AUTHR_MSG auth_msg;
        auth_msg.host = "api.live.bilibili.com";
        auth_msg.port = "443";
        auth_msg.target = "/room/v1/Room/room_init?id=" + roomlist;
        auto resp = sync_https_get<http::string_body>(ioc, ctx, auth_msg, 11).body();
        std::string rid = findvalue(resp, "data", "room_id\":", ",");
        auth_msg.rid = stoi(rid);
        auth_msg.target = "/room/v1/Danmu/getConf?room_id=" + rid + "&platform=pc&player=12176";
        resp = sync_https_get<http::string_body>(ioc, ctx, auth_msg, 11).body();
        auth_msg.key = findvalue(resp, "data", "token\":\"", "\"");
        auth_msg.host = "broadcastlv.chat.bilibili.com";
        auth_msg.target = "/sub";
        // sync request for auth info.
        auto wss_test = std::make_shared<co_websocket>(auth_msg, ioc, ctx);
        netbase::signal_set signals(ioc, SIGINT, SIGTERM);
        netbase::spawn(ioc, std::bind(&co_websocket::co_connect, wss_test, std::placeholders::_1));
        signals.async_wait(
            [&](error_code const &, int)
            {
                wss_test->stop();
                // throw system error 995
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
