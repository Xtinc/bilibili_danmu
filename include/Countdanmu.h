#ifndef COUNTDANMU_H
#define COUNTDANMU_H
#include <unordered_map>
#include <list>
#include <memory>
#include "netapps.h"

namespace danmu
{
    enum class BCMD
    {
        DM_NONE,
        DM_INTERACT_WORD,
        DM_DANMU_MSG,
        DM_STOP_LIVE_ROOM_LIST,
        DM_ONLINE_RANK_COUNT,
        DM_LIVE_INTERACTIVE_GAME,
        DM_SEND_GIFT,
        DM_ROOM_REAL_TIME_MESSAGE_UPDATE,
        DM_ENTRY_EFFECT,
        DM_ONLINE_RANK_V2,
        DM_HOT_RANK_CHANGED,
        DM_HOT_RANK_CHANGED_V2,
        DM_COMBO_SEND,
        DM_WIDGET_BANNER,
        DM_ONLINE_RANK_TOP3,
        DM_SUPER_CHAT_MESSAGE,
        DM_SUPER_CHAT_MESSAGE_JPN,
        DM_NOTICE_MSG,
        DM_HOT_ROOM_NOTIFY
    };
    class Bili_CMD_TYPE
    {
    private:
        inline static std::unordered_map<std::string, BCMD> cmdmap{
            {"DANMU_MSG", BCMD::DM_DANMU_MSG},
            {"INTERACT_WORD", BCMD::DM_INTERACT_WORD},
            {"ONLINE_RANK_COUNT", BCMD::DM_ONLINE_RANK_COUNT},
            {"INTERACT_WORD", BCMD::DM_INTERACT_WORD},
            {"STOP_LIVE_ROOM_LIST", BCMD::DM_STOP_LIVE_ROOM_LIST},
            {"LIVE_INTERACTIVE_GAME", BCMD::DM_LIVE_INTERACTIVE_GAME},
            {"SEND_GIFT", BCMD::DM_SEND_GIFT},
            {"ROOM_REAL_TIME_MESSAGE_UPDATE", BCMD::DM_ROOM_REAL_TIME_MESSAGE_UPDATE},
            {"ENTRY_EFFECT", BCMD::DM_ENTRY_EFFECT},
            {"ONLINE_RANK_V2", BCMD::DM_ONLINE_RANK_V2},
            {"HOT_RANK_CHANGED", BCMD::DM_HOT_RANK_CHANGED},
            {"HOT_RANK_CHANGED_V2", BCMD::DM_HOT_RANK_CHANGED_V2},
            {"COMBO_SEND", BCMD::DM_COMBO_SEND},
            {"WIDGET_BANNER", BCMD::DM_WIDGET_BANNER},
            {"ONLINE_RANK_TOP3", BCMD::DM_ONLINE_RANK_TOP3},
            {"SUPER_CHAT_MESSAGE", BCMD::DM_SUPER_CHAT_MESSAGE},
            {"SUPER_CHAT_MESSAGE_JPN", BCMD::DM_SUPER_CHAT_MESSAGE_JPN},
            {"NOTICE_MSG", BCMD::DM_NOTICE_MSG}};

    public:
        static BCMD Check(const char *cmd_typ)
        {
            try
            {
                return cmdmap.at(std::string(cmd_typ));
            }
            catch (std::exception &)
            {
                return BCMD::DM_NONE;
            }
        };
        static BCMD Check(std::string &cmd_typ)
        {
            try
            {
                return cmdmap.at(cmd_typ);
            }
            catch (std::exception &)
            {
                return BCMD::DM_NONE;
            }
        };
    };

    void ParseJSON(const char *msg, STS_INFO& sts);
}

#endif