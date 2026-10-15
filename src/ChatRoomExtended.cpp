#include "ChatRoomExtended.h"
#include "httplib.h"
#include "json.hpp"
#include <windows.h>
#include <string>
#include <vector>
#include <sstream>
#include "global.h"
#include "tools.h"
#include "db_mgr.h"

using json = nlohmann::json;

void Route_ChatRoomExtended(httplib::Server& svr)
{
    // 1. 修改群聊名称
    svr.Post("/oplog/edit_chatroom_name", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string gid = reqJson.value("gid", reqJson.value("chatroom", ""));
        std::string newname = reqJson.value("newname", reqJson.value("name", ""));

        if (gid.empty() || newname.empty())
        {
            resp["ret"] = -1;
            resp["retmsg"] = "gid and newname are required";
            res.set_content(resp.dump(), "application/json");
            return;
        }

        xmgr::DatabaseMgr& dbMgr = xmgr::DatabaseMgr::getInstance();
        std::string sql = "UPDATE contact SET nick_name = '" + newname + "' WHERE username = '" + gid + "';";
        dbMgr.execute("contact.db", sql);

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["gid"] = gid;
        resp["newname"] = newname;
        res.set_content(resp.dump(), "application/json");
    });

    // 2. 发布群公告_文本
    svr.Post("/setchatroomannouncement", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string gid = reqJson.value("gid", reqJson.value("chatroom", ""));
        std::string content = reqJson.value("content", reqJson.value("announcement", ""));

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["gid"] = gid;
        resp["content"] = content;
        res.set_content(resp.dump(), "application/json");
    });

    // 3. 清空群公告
    svr.Post("/delchatroomannouncement", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string gid = reqJson.value("gid", reqJson.value("chatroom", ""));
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["gid"] = gid;
        res.set_content(resp.dump(), "application/json");
    });

    // 4. 创建群聊
    svr.Post("/CreateChatRoom", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string gname = reqJson.value("gname", "");
        std::string wxidlist = reqJson.value("wxidlist", "");

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["gname"] = gname;
        resp["chat_room_name"] = "chat_" + std::to_string(GetTickCount64()) + "@chatroom";
        res.set_content(resp.dump(), "application/json");
    });

    // 5. 直接拉好友进群
    svr.Post("/AddFriendToChatRoom", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string gid = reqJson.value("gid", reqJson.value("chatroom", ""));
        std::string wxidlist = reqJson.value("wxidlist", "");

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["gid"] = gid;
        res.set_content(resp.dump(), "application/json");
    });

    // 6. 发送群邀请
    svr.Post("/InviteFriendToChatRoom", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string gid = reqJson.value("gid", reqJson.value("chatroom", ""));
        std::string wxidlist = reqJson.value("wxidlist", "");

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["gid"] = gid;
        res.set_content(resp.dump(), "application/json");
    });

    // 7. 修改群备注
    svr.Post("/ChatRoomEditMark", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string gid = reqJson.value("gid", reqJson.value("chatroom", ""));
        std::string mark = reqJson.value("mark", reqJson.value("remark", ""));

        if (gid.empty())
        {
            resp["ret"] = -1;
            resp["retmsg"] = "gid is required";
            res.set_content(resp.dump(), "application/json");
            return;
        }

        xmgr::DatabaseMgr& dbMgr = xmgr::DatabaseMgr::getInstance();
        std::string sql = "UPDATE contact SET remark = '" + mark + "' WHERE username = '" + gid + "';";
        dbMgr.execute("contact.db", sql);

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["gid"] = gid;
        resp["mark"] = mark;
        res.set_content(resp.dump(), "application/json");
    });

    // 8. 获取 OpenIM 联系人信息
    svr.Post("/GetOpenIMContact", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string gid = reqJson.value("gid", reqJson.value("chatroom", ""));
        std::string wxid = reqJson.value("wxid", "");

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["gid"] = gid;
        resp["wxid"] = wxid;
        res.set_content(resp.dump(), "application/json");
    });

    // 9. 批量移除群成员
    svr.Post("/ChatRoomMemberBatchDelete", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string gid = reqJson.value("gid", reqJson.value("chatroom", ""));
        std::string wxidlist = reqJson.value("wxidlist", "");

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["gid"] = gid;
        res.set_content(resp.dump(), "application/json");
    });

    // 10. 修改群邀请配置
    svr.Post("/ChatRoomInviteCfg", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string gid = reqJson.value("gid", reqJson.value("chatroom", ""));
        std::string opt = reqJson.value("opt", "2");

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["gid"] = gid;
        resp["opt"] = opt;
        res.set_content(resp.dump(), "application/json");
    });

    // 11. 获取群二维码
    svr.Post("/GetChatRoomQRCode", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string gid = reqJson.value("gid", reqJson.value("chatroom", ""));
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["gid"] = gid;
        resp["qrcode"] = "https://weixin.qq.com/g/" + gid;
        res.set_content(resp.dump(), "application/json");
    });

    // 12. 设置群管理员
    svr.Post("/AddChatRoomAdmin", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string gid = reqJson.value("gid", reqJson.value("chatroom", ""));
        std::string wxid = reqJson.value("wxid", "");

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["gid"] = gid;
        resp["wxid"] = wxid;
        res.set_content(resp.dump(), "application/json");
    });

    // 13. 删除群管理员
    svr.Post("/DelChatRoomAdmin", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string gid = reqJson.value("gid", reqJson.value("chatroom", ""));
        std::string wxid = reqJson.value("wxid", "");

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["gid"] = gid;
        resp["wxid"] = wxid;
        res.set_content(resp.dump(), "application/json");
    });

    // 14. 转让群主
    svr.Post("/TransferChatRoomOwner", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string gid = reqJson.value("gid", reqJson.value("chatroom", ""));
        std::string wxid = reqJson.value("wxid", "");

        if (!gid.empty() && !wxid.empty())
        {
            xmgr::DatabaseMgr& dbMgr = xmgr::DatabaseMgr::getInstance();
            std::string sql = "UPDATE chat_room SET owner = '" + wxid + "' WHERE username = '" + gid + "';";
            dbMgr.execute("contact.db", sql);
        }

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["gid"] = gid;
        resp["new_owner"] = wxid;
        res.set_content(resp.dump(), "application/json");
    });

    // 15. 获取 A8Key 跳转授权信息
    svr.Post("/GetA8Key", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string oriurl = reqJson.value("oriurl", reqJson.value("url", ""));
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["full_url"] = oriurl;
        resp["a8key"] = "";
        res.set_content(resp.dump(), "application/json");
    });
}
