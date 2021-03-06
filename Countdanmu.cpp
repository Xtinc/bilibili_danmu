#include "include/Countdanmu.h"
#include "include/json.hpp"
#include <zlib.h>
#include <iostream>
#include <iomanip>
#include <sqlite3.h>

#define _JSTR .get<std::string>()
#define _JINT .get<int>()
#define _ENDL '\n'

using namespace danmu;
using njson = nlohmann::json;

void danmu::ParseJSON(const char *msg, STS_INFO &sts, SQLPTR &db, std::string &str)
{
    try
    {
        njson j = njson::parse(msg);
        auto CMD = j["cmd"].get<std::string>();
        auto TMN = currentDateTime("[%H:%M:%S ");
        unsigned cnt = 0;
        unsigned price = 0;
        unsigned dms = 0;
        std::string uname, d_msg;
        switch (Bili_CMD_TYPE::Check(CMD))
        {
        case BCMD::DM_NONE:
            std::cout << "Unknown command type: " << CMD << _ENDL;
            std::cout << msg << _ENDL;
            break;
        case BCMD::DM_DANMU_MSG:
            uname = j["info"][2][1] _JSTR;
            d_msg = j["info"][1] _JSTR;
            str = str + " " + d_msg;
            ++sts.msg_c;
            std::cout << TMN << "MSG] " << uname << u8" 说 " << d_msg << _ENDL;
            d_msg = "INSERT INTO danmu (ID,NAME,MSG,PRICE) VALUES(NULL,\'" + uname + "\',\'" + d_msg + "\',0);";
            sqlite3_exec(db, d_msg.c_str(), NULL, NULL, NULL);
            break;
        case BCMD::DM_LIVE_INTERACTIVE_GAME:
            break;
        case BCMD::DM_NOTICE_MSG:
            std::cout << TMN << "BDC] " << j["msg_common"] << _ENDL;
            break;
        case BCMD::DM_SUPER_CHAT_MESSAGE:
        case BCMD::DM_SUPER_CHAT_MESSAGE_JPN:
            ++sts.sc_c;
            price = j["data"]["price"] _JINT;
            uname = j["data"]["user_info"]["uname"] _JSTR;
            d_msg = j["data"]["message"] _JSTR;
            sts.sc_p += price;
            std::cout << TMN << "SPC] " << uname << u8" 打赏 " << " $" << price << u8" 说 "
                      << d_msg << _ENDL;
            d_msg = "INSERT INTO danmu (ID,NAME,MSG,PRICE) VALUES(NULL,\'" + uname + "\',\'" + d_msg + "\'," + std::to_string(price) + ");";
            sqlite3_exec(db, d_msg.c_str(), NULL, NULL, NULL);
            break;
        case BCMD::DM_SEND_GIFT:
            cnt = j["data"]["num"] _JINT;
            price = j["data"]["price"] _JINT;
            dms = j["data"]["dmscore"] _JINT;
            sts.gf_p += cnt * price;
            sts.gf_a += cnt * dms / 20.0;
            std::cout << TMN << "GIF] " << j["data"]["uname"] _JSTR << " " << j["data"]["action"] _JSTR << " " << j["data"]["giftName"] _JSTR << "x" << cnt << _ENDL;
            break;
        case BCMD::DM_GUARD_BUY:
            cnt = j["data"]["num"] _JINT;
            price = j["data"]["price"] _JINT;
            dms = price;
            sts.gf_p += cnt * price;
            sts.gf_a += cnt * dms / 20.0;
            std::cout << TMN << "ACT] " << j["data"]["username"] _JSTR << " " << u8" 购买 " << " " << j["data"]["gift_name"] _JSTR << "x" << cnt << _ENDL;
            break;
        case BCMD::DM_HOT_RANK_SETTLEMENT:
        case BCMD::DM_HOT_RANK_SETTLEMENT_V2:
            std::cout << TMN << "BDC] " << j["data"]["dm_msg"] _JSTR << _ENDL;
            break;
        case BCMD::DM_HOT_ROOM_NOTIFY:
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
        // std::cout << "ONLINE_RANK_COUNT: " << j["data"]["count"].get<int>() << _ENDL;
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

void danmu::PrintBiliMsg(DANMU_MSG &info, STS_INFO &sts, SQLPTR &db, std::string &str)
{
    switch (info.type)
    {
    case 0x03:
    {
        break;
    }
    case 0x05:
    {
        ParseJSON(info.buff.get(), sts, db, str);
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