#ifndef COUNTDANMU_H
#define COUNTDANMU_H
#include <unordered_map>
#include "include/symbols.h"

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
        DM_HOT_ROOM_NOTIFY,
        DM_GUARD_BUY,
        DM_ANCHOR_LOT_END,
        DM_ANCHOR_LOT_AWARD,
        DM_USER_TOAST_MSG,
        DM_HOT_RANK_SETTLEMENT,
        DM_HOT_RANK_SETTLEMENT_V2,
        DM_PK_BATTLE_PROCESS,
        DM_PK_BATTLE_PROCESS_NEW,
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
            {"NOTICE_MSG", BCMD::DM_NOTICE_MSG},
            {"GUARD_BUY", BCMD::DM_GUARD_BUY},
            {"HOT_ROOM_NOTIFY", BCMD::DM_HOT_ROOM_NOTIFY},
            {"HOT_RANK_SETTLEMENT", BCMD::DM_HOT_RANK_SETTLEMENT},
            {"HOT_RANK_SETTLEMENT_V2", BCMD::DM_HOT_RANK_SETTLEMENT_V2},
            {"PK_BATTLE_PROCESS_NEW", BCMD::DM_PK_BATTLE_PROCESS_NEW},
            {"PK_BATTLE_PROCESS", BCMD::DM_PK_BATTLE_PROCESS}};

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

    void ParseJSON(const char *msg, STS_INFO &sts, SQLPTR &db);

    void PrintBiliMsg(DANMU_MSG &info, STS_INFO &sts, SQLPTR &db);

    class Parser
    {
    private:
        std::string remained;
        DANMU_HANDLE uhandler;

    public:
        Parser(DANMU_HANDLE &&uhd);
        ~Parser();
        Parser(const Parser &) = delete;
        Parser(Parser &&) = delete;
        Parser operator=(const Parser &) = delete;
        Parser operator=(Parser &&) = delete;

        void parser(const std::string &msg, size_t bytf);

    private:
        void process_data(const char *buf, unsigned int ilen, unsigned int typ);
    };
}

#endif