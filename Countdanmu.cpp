#include "include/Countdanmu.h"
#include "include/json.hpp"
#include "include/netapps.h"

#define _JSTR .get<std::string>()
#define _JINT .get<int>()

using namespace danmu;
using njson = nlohmann::json;

void danmu::ParseJSON(const char *msg, STS_INFO& sts)
{
    try
    {
        njson j = njson::parse(msg);
        auto CMD = j["cmd"].get<std::string>();
        auto TMN = currentDateTime("[%H:%M:%S ");
        unsigned cnt = 0;
        unsigned price = 0;
        unsigned dms = 0;
        switch (Bili_CMD_TYPE::Check(CMD))
        {
        case BCMD::DM_NONE:
            std::cout << "Unknown command type: " << CMD << std::endl;
            break;
        case BCMD::DM_DANMU_MSG:
            ++sts.msg_c;
            std::cout << TMN << "MSG] " << j["info"][2][1] _JSTR << u8" 说 " << j["info"][1] _JSTR << std::endl;
            break;
        case BCMD::DM_LIVE_INTERACTIVE_GAME:
            //? 作用？
            // std::cout << "[ACT] " << j["data"]["uname"] _JSTR << u8" 投喂 " << j["data"]["gift_name"] _JSTR << "x" << j["data"]["gift_num"] _JINT << std::endl;
            break;
        case BCMD::DM_NOTICE_MSG:
            std::cout << TMN << "BDC] " << j["msg_common"] << std::endl;
            break;
        case BCMD::DM_SUPER_CHAT_MESSAGE:
        case BCMD::DM_SUPER_CHAT_MESSAGE_JPN:
            ++sts.sc_c;
            price = j["data"]["price"] _JINT;
            sts.sc_p += price;
            std::cout << TMN << "SPC] " << j["data"]["user_info"]["uname"] _JSTR << u8" 打赏 " << " $" << price << u8" 说 "
                      << j["data"]["message"] _JSTR << std::endl;
            break;
        case BCMD::DM_SEND_GIFT:
            cnt = j["data"]["num"] _JINT;
            price = j["data"]["price"] _JINT;
            dms = j["data"]["dmscore"] _JINT;
            sts.gf_p += cnt * price;
            sts.gf_a += cnt * dms / 20.0;
            std::cout << TMN << "GIF] " << j["data"]["uname"] _JSTR << " " << j["data"]["action"] _JSTR << " " << j["data"]["giftName"] _JSTR << "x" << cnt << std::endl;
            break;
        //?
        case BCMD::DM_COMBO_SEND:
        // 连续送礼触发
        case BCMD::DM_HOT_RANK_CHANGED:
        case BCMD::DM_HOT_RANK_CHANGED_V2:
        // 主播排名
        case BCMD::DM_ONLINE_RANK_V2:
        // 高能榜
        case BCMD::DM_ROOM_REAL_TIME_MESSAGE_UPDATE:
        // 房间信息更新
        case BCMD::DM_ENTRY_EFFECT:
        // 舰长进入
        case BCMD::DM_ONLINE_RANK_COUNT:
        // std::cout << "ONLINE_RANK_COUNT: " << j["data"]["count"].get<int>() << std::endl;
        case BCMD::DM_STOP_LIVE_ROOM_LIST:
        case BCMD::DM_INTERACT_WORD:
            // 普通用户进入
            break;
        default:
            break;
        }
    }
    catch (std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        return;
    }
}