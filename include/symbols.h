#ifndef SYMBOL_H
#define SYMBOL_H

#include <string>
#include <memory>
#include <functional>
#include <ctime>

namespace danmu
{
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
}
#endif