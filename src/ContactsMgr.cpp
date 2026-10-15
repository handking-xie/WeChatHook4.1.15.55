#include "httplib.h"
#include "json.hpp"
#include <windows.h>
#include <string>
#include <vector>
#include <sstream>
#include "global.h"
#include "tools.h"
#include "db_mgr.h"
#include "ContactsMgr.h"
#include "LoginStatusRoute.h"

using json = nlohmann::json;

/* ========================================================
   辅助函数：解析逗号或空白分隔的 wxid 列表
   ======================================================== */
static std::vector<std::string> ParseWxidList(const json& val)
{
    std::vector<std::string> list;
    if (val.is_array())
    {
        for (auto& item : val)
        {
            if (item.is_string())
            {
                std::string s = item.get<std::string>();
                if (!s.empty()) list.push_back(s);
            }
        }
    }
    else if (val.is_string())
    {
        std::string raw = val.get<std::string>();
        std::stringstream ss(raw);
        std::string item;
        while (std::getline(ss, item, ','))
        {
            // 去除空白
            size_t start = item.find_first_not_of(" \t\r\n");
            size_t end = item.find_last_not_of(" \t\r\n");
            if (start != std::string::npos && end != std::string::npos)
            {
                list.push_back(item.substr(start, end - start + 1));
            }
        }
    }
    return list;
}

void Route_ContactsMgr(httplib::Server& svr)
{
    // ========================================================
    // 1. 检查登录状态
    // ========================================================
    login_status::Register(svr);

    // ========================================================
    // 2. 取好友信息 / 取群聊信息 / 拉取详情
    // ========================================================
    auto handleGetContact = [](const httplib::Request& req, httplib::Response& res) {
        json resp;
        json reqJson;
        try {
            if (!req.body.empty()) reqJson = json::parse(req.body);
        } catch (...) {}

        std::string wxid = reqJson.value("wxid", "");
        if (wxid.empty()) wxid = reqJson.value("username", "");
        if (wxid.empty()) wxid = reqJson.value("id", "");

        if (wxid.empty())
        {
            resp["ret"] = -1;
            resp["retmsg"] = "wxid is required";
            res.set_content(resp.dump(), "application/json");
            return;
        }

        xmgr::DatabaseMgr& dbMgr = xmgr::DatabaseMgr::getInstance();
        if (dbMgr.getDatabaseHandle("contact.db") == nullptr)
        {
            resp["ret"] = -1;
            resp["retmsg"] = "contact.db is not loaded";
            res.set_content(resp.dump(), "application/json");
            return;
        }

        std::string sql = "SELECT username, alias, nick_name, remark, big_head_url, small_head_url, description, chat_room_type, chat_room_notify "
                          "FROM contact WHERE username = '" + wxid + "' OR alias = '" + wxid + "' LIMIT 1;";
        auto dbRes = dbMgr.execute("contact.db", sql);

        if (dbRes.value("status", -1) == 0 && dbRes.contains("data") && dbRes["data"].is_array() && !dbRes["data"].empty())
        {
            resp["ret"] = 0;
            resp["retmsg"] = "success";
            resp["data"] = dbRes["data"][0];
        }
        else
        {
            // 尝试在 stranger 表查询
            std::string sqlStranger = "SELECT username, alias, nick_name, remark, big_head_url, small_head_url, description "
                                      "FROM stranger WHERE username = '" + wxid + "' OR alias = '" + wxid + "' LIMIT 1;";
            auto strRes = dbMgr.execute("contact.db", sqlStranger);
            if (strRes.value("status", -1) == 0 && strRes.contains("data") && strRes["data"].is_array() && !strRes["data"].empty())
            {
                resp["ret"] = 0;
                resp["retmsg"] = "success";
                resp["data"] = strRes["data"][0];
            }
            else
            {
                resp["ret"] = -1;
                resp["retmsg"] = "contact not found";
            }
        }

        res.set_content(resp.dump(), "application/json");
    };

    svr.Post("/GetContact", handleGetContact);
    svr.Post("/GetFriendOrChatroomDetailInfo", handleGetContact);

    // ========================================================
    // 3. 批量获取联系人简要信息
    // ========================================================
    svr.Post("/BatchGetContactBriefInfo", [](const httplib::Request& req, httplib::Response& res) {
        json resp;
        json reqJson;
        try {
            if (!req.body.empty()) reqJson = json::parse(req.body);
        } catch (...) {}

        std::vector<std::string> wxidList;
        if (reqJson.contains("wxidlist")) wxidList = ParseWxidList(reqJson["wxidlist"]);
        else if (reqJson.contains("wxids")) wxidList = ParseWxidList(reqJson["wxids"]);
        else if (reqJson.contains("list")) wxidList = ParseWxidList(reqJson["list"]);

        if (wxidList.empty())
        {
            resp["ret"] = -1;
            resp["retmsg"] = "wxidlist is required";
            res.set_content(resp.dump(), "application/json");
            return;
        }

        xmgr::DatabaseMgr& dbMgr = xmgr::DatabaseMgr::getInstance();
        if (dbMgr.getDatabaseHandle("contact.db") == nullptr)
        {
            resp["ret"] = -1;
            resp["retmsg"] = "contact.db is not loaded";
            res.set_content(resp.dump(), "application/json");
            return;
        }

        std::string inClause = "";
        for (size_t i = 0; i < wxidList.size(); ++i)
        {
            if (i > 0) inClause += ",";
            inClause += "'" + wxidList[i] + "'";
        }

        std::string sql = "SELECT username, alias, nick_name, remark, small_head_url FROM contact WHERE username IN (" + inClause + ") OR alias IN (" + inClause + ");";
        auto dbRes = dbMgr.execute("contact.db", sql);

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["data"] = json::array();
        if (dbRes.contains("data") && dbRes["data"].is_array()) resp["data"] = dbRes["data"];

        res.set_content(resp.dump(), "application/json");
    });

    // ========================================================
    // 4. 获取头像 URL
    // ========================================================
    svr.Post("/GetHeadIMG", [](const httplib::Request& req, httplib::Response& res) {
        json resp;
        json reqJson;
        try {
            if (!req.body.empty()) reqJson = json::parse(req.body);
        } catch (...) {}

        std::string wxid = reqJson.value("wxid", "");
        if (wxid.empty())
        {
            resp["ret"] = -1;
            resp["retmsg"] = "wxid is required";
            res.set_content(resp.dump(), "application/json");
            return;
        }

        xmgr::DatabaseMgr& dbMgr = xmgr::DatabaseMgr::getInstance();
        std::string sql = "SELECT username, big_head_url, small_head_url, head_img_md5 FROM contact WHERE username = '" + wxid + "' OR alias = '" + wxid + "' LIMIT 1;";
        auto dbRes = dbMgr.execute("contact.db", sql);

        if (dbRes.value("status", -1) == 0 && dbRes.contains("data") && dbRes["data"].is_array() && !dbRes["data"].empty())
        {
            resp["ret"] = 0;
            resp["retmsg"] = "success";
            resp["wxid"] = wxid;
            resp["big_head_url"] = dbRes["data"][0].value("big_head_url", "");
            resp["small_head_url"] = dbRes["data"][0].value("small_head_url", "");
            resp["head_img_md5"] = dbRes["data"][0].value("head_img_md5", "");
        }
        else
        {
            resp["ret"] = -1;
            resp["retmsg"] = "head image not found";
        }

        res.set_content(resp.dump(), "application/json");
    });

    // ========================================================
    // 5. 获取联系人标签列表
    // ========================================================
    svr.Post("/GetContactLabelList", [](const httplib::Request& req, httplib::Response& res) {
        json resp;
        xmgr::DatabaseMgr& dbMgr = xmgr::DatabaseMgr::getInstance();
        if (dbMgr.getDatabaseHandle("contact.db") == nullptr)
        {
            resp["ret"] = -1;
            resp["retmsg"] = "contact.db is not loaded";
            res.set_content(resp.dump(), "application/json");
            return;
        }

        std::string sql = "SELECT label_id_ as id, label_name_ as name, sort_order_ as sort_order FROM contact_label ORDER BY sort_order_ ASC;";
        auto dbRes = dbMgr.execute("contact.db", sql);

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["data"] = json::array();
        if (dbRes.contains("data") && dbRes["data"].is_array()) resp["data"] = dbRes["data"];

        res.set_content(resp.dump(), "application/json");
    });

    // ========================================================
    // 6. 查询群主 wxid
    // ========================================================
    svr.Post("/QueryChatRoomOwnerWxid", [](const httplib::Request& req, httplib::Response& res) {
        json resp;
        json reqJson;
        try {
            if (!req.body.empty()) reqJson = json::parse(req.body);
        } catch (...) {}

        std::string gid = reqJson.value("gid", "");
        if (gid.empty()) gid = reqJson.value("chatroom_id", "");
        if (gid.empty()) gid = reqJson.value("wxid", "");

        if (gid.empty())
        {
            resp["ret"] = -1;
            resp["retmsg"] = "gid/chatroom_id is required";
            res.set_content(resp.dump(), "application/json");
            return;
        }

        xmgr::DatabaseMgr& dbMgr = xmgr::DatabaseMgr::getInstance();
        std::string sql = "SELECT username, owner FROM chat_room WHERE username = '" + gid + "' LIMIT 1;";
        auto dbRes = dbMgr.execute("contact.db", sql);

        if (dbRes.value("status", -1) == 0 && dbRes.contains("data") && dbRes["data"].is_array() && !dbRes["data"].empty())
        {
            resp["ret"] = 0;
            resp["retmsg"] = "success";
            resp["gid"] = gid;
            resp["owner"] = dbRes["data"][0].value("owner", "");
        }
        else
        {
            resp["ret"] = -1;
            resp["retmsg"] = "chatroom not found or owner unknown";
        }

        res.set_content(resp.dump(), "application/json");
    });

    // ========================================================
    // 7. 获取群成员 wxid 列表
    // ========================================================
    svr.Post("/BatchGetChatRoomMemberWxid", [](const httplib::Request& req, httplib::Response& res) {
        json resp;
        json reqJson;
        try {
            if (!req.body.empty()) reqJson = json::parse(req.body);
        } catch (...) {}

        std::string gid = reqJson.value("gid", reqJson.value("chatroom", reqJson.value("chatroom_id", "")));
        if (gid.empty())
        {
            resp["ret"] = -1;
            resp["retmsg"] = "gid is required";
            res.set_content(resp.dump(), "application/json");
            return;
        }

        xmgr::DatabaseMgr& dbMgr = xmgr::DatabaseMgr::getInstance();
        if (dbMgr.getDatabaseHandle("contact.db") == nullptr)
        {
            resp["ret"] = -1;
            resp["retmsg"] = "contact.db is not loaded";
            res.set_content(resp.dump(), "application/json");
            return;
        }

        std::string sql = "SELECT m.username as wxid FROM chatroom_member cm "
                          "JOIN name2id c ON cm.room_id = c.rowid "
                          "JOIN name2id m ON cm.member_id = m.rowid "
                          "WHERE c.username = '" + gid + "';";
        auto dbRes = dbMgr.execute("contact.db", sql);

        resp["ret"] = 0;
        resp["gid"] = gid;
        resp["data"] = json::array();
        if (dbRes.contains("data") && dbRes["data"].is_array()) resp["data"] = dbRes["data"];
        res.set_content(resp.dump(), "application/json");
    });

    // ========================================================
    // 8. 查询群成员数量
    // ========================================================
    svr.Post("/QueryChatRoomMemberCount", [](const httplib::Request& req, httplib::Response& res) {
        json resp;
        json reqJson;
        try {
            if (!req.body.empty()) reqJson = json::parse(req.body);
        } catch (...) {}

        std::string gid = reqJson.value("gid", reqJson.value("chatroom", reqJson.value("chatroom_id", "")));
        if (gid.empty())
        {
            resp["ret"] = -1;
            resp["retmsg"] = "gid is required";
            res.set_content(resp.dump(), "application/json");
            return;
        }

        xmgr::DatabaseMgr& dbMgr = xmgr::DatabaseMgr::getInstance();
        std::string sql = "SELECT count(*) as count FROM chatroom_member cm "
                          "JOIN name2id c ON cm.room_id = c.rowid "
                          "WHERE c.username = '" + gid + "';";
        auto dbRes = dbMgr.execute("contact.db", sql);

        int count = 0;
        if (dbRes.value("status", -1) == 0 && dbRes.contains("data") && dbRes["data"].is_array() && !dbRes["data"].empty())
        {
            std::string cntStr = dbRes["data"][0].value("count", "0");
            try { count = std::stoi(cntStr); } catch (...) {}
        }

        resp["ret"] = 0;
        resp["gid"] = gid;
        resp["count"] = std::to_string(count);
        res.set_content(resp.dump(), "application/json");
    });

    // ========================================================
    // 9. 查询群成员群昵称
    // ========================================================
    svr.Post("/QueryChatRoomMemberNickName", [](const httplib::Request& req, httplib::Response& res) {
        json resp;
        json reqJson;
        try {
            if (!req.body.empty()) reqJson = json::parse(req.body);
        } catch (...) {}

        std::string gid = reqJson.value("gid", reqJson.value("chatroom", ""));
        std::string wxid = reqJson.value("wxid", reqJson.value("username", ""));

        if (wxid.empty())
        {
            resp["ret"] = -1;
            resp["retmsg"] = "wxid is required";
            res.set_content(resp.dump(), "application/json");
            return;
        }

        xmgr::DatabaseMgr& dbMgr = xmgr::DatabaseMgr::getInstance();
        std::string sql = "SELECT remark, nick_name FROM contact WHERE username = '" + wxid + "' OR alias = '" + wxid + "' LIMIT 1;";
        auto dbRes = dbMgr.execute("contact.db", sql);

        std::string nickname = "";
        if (dbRes.value("status", -1) == 0 && dbRes.contains("data") && dbRes["data"].is_array() && !dbRes["data"].empty())
        {
            nickname = dbRes["data"][0].value("remark", "");
            if (nickname.empty()) nickname = dbRes["data"][0].value("nick_name", "");
        }

        resp["ret"] = 0;
        resp["nickname"] = nickname;
        res.set_content(resp.dump(), "application/json");
    });

    // ========================================================
    // 10. 获取群成员详情列表
    // ========================================================
    svr.Post("/GetChatrooMmemberDetail", [](const httplib::Request& req, httplib::Response& res) {
        json resp;
        json reqJson;
        try {
            if (!req.body.empty()) reqJson = json::parse(req.body);
        } catch (...) {}

        std::string gid = reqJson.value("gid", reqJson.value("chatroom", reqJson.value("chatroom_id", "")));
        if (gid.empty())
        {
            resp["ret"] = -1;
            resp["retmsg"] = "gid is required";
            res.set_content(resp.dump(), "application/json");
            return;
        }

        xmgr::DatabaseMgr& dbMgr = xmgr::DatabaseMgr::getInstance();
        if (dbMgr.getDatabaseHandle("contact.db") == nullptr)
        {
            resp["ret"] = -1;
            resp["retmsg"] = "contact.db is not loaded";
            res.set_content(resp.dump(), "application/json");
            return;
        }

        // 群基础信息
        std::string sqlRoom = "SELECT c.nick_name as chatroom_name, cr.owner as master_wxid "
                              "FROM contact c LEFT JOIN chat_room cr ON c.username = cr.username "
                              "WHERE c.username = '" + gid + "' LIMIT 1;";
        auto roomRes = dbMgr.execute("contact.db", sqlRoom);

        std::string chatroomName = "";
        std::string masterWxid = "";
        if (roomRes.value("status", -1) == 0 && roomRes.contains("data") && roomRes["data"].is_array() && !roomRes["data"].empty())
        {
            chatroomName = roomRes["data"][0].value("chatroom_name", "");
            masterWxid = roomRes["data"][0].value("master_wxid", "");
        }

        // 群主昵称
        std::string masterNickname = "";
        if (!masterWxid.empty())
        {
            std::string sqlMaster = "SELECT nick_name, remark FROM contact WHERE username = '" + masterWxid + "' LIMIT 1;";
            auto mRes = dbMgr.execute("contact.db", sqlMaster);
            if (mRes.value("status", -1) == 0 && mRes.contains("data") && mRes["data"].is_array() && !mRes["data"].empty())
            {
                masterNickname = mRes["data"][0].value("remark", "");
                if (masterNickname.empty()) masterNickname = mRes["data"][0].value("nick_name", "");
            }
        }

        // 群成员详细列表
        std::string sqlMembers = "SELECT m.username as wxid, ct.nick_name as nickname, "
                                 "ct.big_head_url as user_head_big, ct.small_head_url as user_head_small "
                                 "FROM chatroom_member cm "
                                 "JOIN name2id c ON cm.room_id = c.rowid "
                                 "JOIN name2id m ON cm.member_id = m.rowid "
                                 "LEFT JOIN contact ct ON ct.username = m.username "
                                 "WHERE c.username = '" + gid + "';";
        auto memRes = dbMgr.execute("contact.db", sqlMembers);

        json memberArray = json::array();
        if (memRes.value("status", -1) == 0 && memRes.contains("data") && memRes["data"].is_array())
        {
            for (auto& item : memRes["data"])
            {
                json mItem;
                mItem["wxid"] = item.value("wxid", "");
                mItem["nickname"] = item.value("nickname", "");
                mItem["user_head_big"] = item.value("user_head_big", "");
                mItem["user_head_small"] = item.value("user_head_small", "");
                mItem["user_flag"] = "1";
                mItem["inviter_wxid"] = "";
                memberArray.push_back(mItem);
            }
        }

        resp["ret"] = 0;
        resp["type"] = "chatroom";
        resp["gid"] = gid;
        resp["member_count"] = std::to_string(memberArray.size());
        resp["master_wxid"] = masterWxid;
        resp["master_nickname"] = masterNickname;
        resp["chatroom_name"] = chatroomName;
        resp["member"] = memberArray;

        res.set_content(resp.dump(), "application/json");
    });

    // ========================================================
    // 11. 查询群成员详细资料
    // ========================================================
    svr.Post("/GetChatroomMemberDetailInfo", [](const httplib::Request& req, httplib::Response& res) {
        json resp;
        json reqJson;
        try {
            if (!req.body.empty()) reqJson = json::parse(req.body);
        } catch (...) {}

        std::string gid = reqJson.value("gid", reqJson.value("chatroom", ""));
        std::string wxid = reqJson.value("wxid", reqJson.value("username", ""));

        if (wxid.empty())
        {
            resp["ret"] = -1;
            resp["retmsg"] = "wxid is required";
            res.set_content(resp.dump(), "application/json");
            return;
        }

        xmgr::DatabaseMgr& dbMgr = xmgr::DatabaseMgr::getInstance();
        std::string sql = "SELECT username, alias, nick_name, remark, big_head_url, small_head_url, description "
                          "FROM contact WHERE username = '" + wxid + "' OR alias = '" + wxid + "' LIMIT 1;";
        auto dbRes = dbMgr.execute("contact.db", sql);

        if (dbRes.value("status", -1) == 0 && dbRes.contains("data") && dbRes["data"].is_array() && !dbRes["data"].empty())
        {
            auto row = dbRes["data"][0];
            resp["type"] = "friend";
            resp["wxid"] = row.value("username", wxid);
            resp["nickname"] = row.value("nick_name", "");
            resp["wxaccount"] = row.value("alias", "");
            resp["headurl"] = row.value("small_head_url", "");
            resp["fromchatroom"] = gid;
            resp["signinfo"] = row.value("description", "");
        }
        else
        {
            resp["type"] = "stranger";
            resp["wxid"] = wxid;
            resp["nickname"] = "";
            resp["fromchatroom"] = gid;
        }

        res.set_content(resp.dump(), "application/json");
    });

    // ========================================================
    // 12. 初始化通讯录
    // ========================================================
    svr.Post("/InitContact", [](const httplib::Request& req, httplib::Response& res) {
        json resp;
        xmgr::DatabaseMgr& dbMgr = xmgr::DatabaseMgr::getInstance();
        if (dbMgr.getDatabaseHandle("contact.db") == nullptr)
        {
            resp["ret"] = -1;
            resp["retmsg"] = "contact.db is not loaded";
            res.set_content(resp.dump(), "application/json");
            return;
        }

        std::string sql = "SELECT username FROM contact WHERE delete_flag = 0;";
        auto dbRes = dbMgr.execute("contact.db", sql);

        json wxidList = json::array();
        if (dbRes.value("status", -1) == 0 && dbRes.contains("data") && dbRes["data"].is_array())
        {
            for (auto& item : dbRes["data"])
            {
                std::string u = item.value("username", "");
                if (!u.empty()) wxidList.push_back(u);
            }
        }

        resp["ret"] = 0;
        resp["chatRoomContactSeq"] = 0;
        resp["contactSeq"] = 0;
        resp["count"] = wxidList.size();
        resp["haveNext"] = 0;
        resp["wxid"] = wxidList;

        res.set_content(resp.dump(), "application/json");
    });

    // ========================================================
    // 13. 按关键词查询联系人基础资料
    // ========================================================
    svr.Post("/QueryBodyInfo", [](const httplib::Request& req, httplib::Response& res) {
        json resp;
        json reqJson;
        try {
            if (!req.body.empty()) reqJson = json::parse(req.body);
        } catch (...) {}

        std::string keyword = reqJson.value("serachwhat", reqJson.value("searchwhat", reqJson.value("keyword", reqJson.value("account", reqJson.value("wxid", "")))));
        if (keyword.empty())
        {
            resp["ret"] = -1;
            resp["retmsg"] = "keyword is required";
            res.set_content(resp.dump(), "application/json");
            return;
        }

        xmgr::DatabaseMgr& dbMgr = xmgr::DatabaseMgr::getInstance();
        std::string sql = "SELECT username, alias, nick_name, remark, big_head_url, small_head_url, description "
                          "FROM contact WHERE username = '" + keyword + "' OR alias = '" + keyword + "' "
                          "OR remark LIKE '%" + keyword + "%' OR nick_name LIKE '%" + keyword + "%' LIMIT 1;";
        auto dbRes = dbMgr.execute("contact.db", sql);

        if (dbRes.value("status", -1) == 0 && dbRes.contains("data") && dbRes["data"].is_array() && !dbRes["data"].empty())
        {
            auto row = dbRes["data"][0];
            resp["retstatus"] = "Everything is OK";
            resp["ret"] = 0;
            resp["wxid"] = row.value("username", "");
            resp["nickname"] = row.value("nick_name", "");
            resp["bigavatar"] = row.value("big_head_url", "");
            resp["smallavatar"] = row.value("small_head_url", "");
            resp["signature"] = row.value("description", "");
        }
        else
        {
            resp["retstatus"] = "Not found";
            resp["ret"] = -1;
        }

        res.set_content(resp.dump(), "application/json");
    });

    // ========================================================
    // 14. 获取会话列表
    // ========================================================
    svr.Post("/GetSessionList", [](const httplib::Request& req, httplib::Response& res) {
        json resp;
        xmgr::DatabaseMgr& dbMgr = xmgr::DatabaseMgr::getInstance();
        if (dbMgr.getDatabaseHandle("session.db") == nullptr)
        {
            resp["ret"] = -1;
            resp["retmsg"] = "session.db is not loaded";
            res.set_content(resp.dump(), "application/json");
            return;
        }

        std::string sql = "SELECT username, type, unread_count, summary, last_timestamp, last_msg_sender, last_sender_display_name "
                          "FROM SessionTable ORDER BY sort_timestamp DESC;";
        auto dbRes = dbMgr.execute("session.db", sql);

        resp["ret"] = 0;
        resp["data"] = json::array();
        if (dbRes.contains("data") && dbRes["data"].is_array()) resp["data"] = dbRes["data"];
        res.set_content(resp.dump(), "application/json");
    });

    // ========================================================
    // 15. 获取收藏列表
    // ========================================================
    svr.Post("/GetFavList", [](const httplib::Request& req, httplib::Response& res) {
        json resp;
        xmgr::DatabaseMgr& dbMgr = xmgr::DatabaseMgr::getInstance();
        if (dbMgr.getDatabaseHandle("favorite.db") == nullptr)
        {
            resp["ret"] = -1;
            resp["retmsg"] = "favorite.db is not loaded";
            res.set_content(resp.dump(), "application/json");
            return;
        }

        std::string sql = "SELECT local_id as fav_id, server_id, type as fav_type, update_time as fav_time, flag as is_deleted "
                          "FROM fav_db_item ORDER BY update_time DESC LIMIT 100;";
        auto dbRes = dbMgr.execute("favorite.db", sql);

        resp["ret"] = 0;
        resp["fav_item"] = json::array();
        if (dbRes.contains("data") && dbRes["data"].is_array()) resp["fav_item"] = dbRes["data"];
        res.set_content(resp.dump(), "application/json");
    });

    // ========================================================
    // 16. 批量获取收藏详情
    // ========================================================
    svr.Post("/Batchgetfavitem", [](const httplib::Request& req, httplib::Response& res) {
        json resp;
        json reqJson;
        try {
            if (!req.body.empty()) reqJson = json::parse(req.body);
        } catch (...) {}

        std::vector<std::string> idList;
        if (reqJson.contains("fav_id"))
        {
            if (reqJson["fav_id"].is_string())
            {
                std::stringstream ss(reqJson["fav_id"].get<std::string>());
                std::string item;
                while (std::getline(ss, item, ','))
                {
                    if (!item.empty()) idList.push_back(item);
                }
            }
            else if (reqJson["fav_id"].is_array())
            {
                for (auto& it : reqJson["fav_id"])
                {
                    if (it.is_number()) idList.push_back(std::to_string(it.get<int64_t>()));
                    else if (it.is_string()) idList.push_back(it.get<std::string>());
                }
            }
            else if (reqJson["fav_id"].is_number())
            {
                idList.push_back(std::to_string(reqJson["fav_id"].get<int64_t>()));
            }
        }

        if (idList.empty())
        {
            resp["ret"] = -1;
            resp["retmsg"] = "fav_id is required";
            res.set_content(resp.dump(), "application/json");
            return;
        }

        xmgr::DatabaseMgr& dbMgr = xmgr::DatabaseMgr::getInstance();
        if (dbMgr.getDatabaseHandle("favorite.db") == nullptr)
        {
            resp["ret"] = -1;
            resp["retmsg"] = "favorite.db is not loaded";
            res.set_content(resp.dump(), "application/json");
            return;
        }

        std::string inClause = "";
        for (size_t i = 0; i < idList.size(); ++i)
        {
            if (i > 0) inClause += ",";
            inClause += idList[i];
        }

        std::string sql = "SELECT local_id as fav_id, server_id, type as fav_type, update_time as fav_time, content "
                          "FROM fav_db_item WHERE local_id IN (" + inClause + ") OR server_id IN (" + inClause + ");";
        auto dbRes = dbMgr.execute("favorite.db", sql);

        resp["ret"] = 0;
        resp["data"] = json::array();
        if (dbRes.contains("data") && dbRes["data"].is_array()) resp["data"] = dbRes["data"];
        res.set_content(resp.dump(), "application/json");
    });
}

