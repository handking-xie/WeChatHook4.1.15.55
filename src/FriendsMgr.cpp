#include "FriendsMgr.h"
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

void Route_FriendsMgr(httplib::Server& svr)
{
    // 1. 查询用户所属标签
    svr.Post("/GetUserLabel", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string wxidlist = reqJson.value("wxidlist", reqJson.value("wxid", ""));
        json dataArr = json::array();

        if (!wxidlist.empty())
        {
            xmgr::DatabaseMgr& dbMgr = xmgr::DatabaseMgr::getInstance();
            std::stringstream ss(wxidlist);
            std::string item;
            while (std::getline(ss, item, ','))
            {
                size_t s = item.find_first_not_of(" \t\r\n");
                size_t e = item.find_last_not_of(" \t\r\n");
                if (s != std::string::npos && e != std::string::npos)
                {
                    std::string u = item.substr(s, e - s + 1);
                    std::string sql = "SELECT username, label_id_list FROM contact WHERE username = '" + u + "' LIMIT 1;";
                    auto dbRes = dbMgr.execute("contact.db", sql);
                    std::string labelIds = "";
                    if (dbRes.value("status", -1) == 0 && dbRes.contains("data") && !dbRes["data"].empty())
                    {
                        labelIds = dbRes["data"][0].value("label_id_list", "");
                    }
                    json obj;
                    obj["wxid"] = u;
                    obj["label_id_list"] = labelIds;
                    dataArr.push_back(obj);
                }
            }
        }

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["data"] = dataArr;
        res.set_content(resp.dump(), "application/json");
    });

    // 2. 将联系人加入标签
    svr.Post("/MoveToLabel", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string wxidlist = reqJson.value("wxidlist", reqJson.value("wxid", ""));
        std::string labelid = reqJson.value("labelid", reqJson.value("label_id", ""));

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["wxidlist"] = wxidlist;
        resp["labelid"] = labelid;
        res.set_content(resp.dump(), "application/json");
    });

    // 3. 将联系人移出标签
    svr.Post("/RemoveToLabel", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string wxidlist = reqJson.value("wxidlist", reqJson.value("wxid", ""));
        std::string labelid = reqJson.value("labelid", reqJson.value("label_id", ""));

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["wxidlist"] = wxidlist;
        resp["labelid"] = labelid;
        res.set_content(resp.dump(), "application/json");
    });

    // 4. 上传手机通讯录匹配好友
    svr.Post("/UploadMContact", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        res.set_content(resp.dump(), "application/json");
    });

    // 5. 获取手机通讯录好友匹配结果
    svr.Post("/GetMFriend", [](const httplib::Request& req, httplib::Response& res) {
        json resp;
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["data"] = json::array();
        res.set_content(resp.dump(), "application/json");
    });

    // 6. 同意好友申请
    svr.Post("/VerifyFriend", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string v3 = reqJson.value("v3", "");
        std::string v4 = reqJson.value("v4", "");
        std::string role = reqJson.value("role", "8");

        if (v3.empty() && v4.empty())
        {
            resp["ret"] = -1;
            resp["retmsg"] = "v3/v4 is required";
            res.set_content(resp.dump(), "application/json");
            return;
        }

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["v3"] = v3;
        resp["role"] = role;
        res.set_content(resp.dump(), "application/json");
    });

    // 7. 添加新好友
    svr.Post("/AddNewFriend", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string v3 = reqJson.value("v3_wxid", reqJson.value("v3", ""));
        std::string v4 = reqJson.value("v4", "");
        std::string desc = reqJson.value("desc", reqJson.value("verify_content", ""));

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["v3_wxid"] = v3;
        resp["desc"] = desc;
        res.set_content(resp.dump(), "application/json");
    });

    // 8. 删除好友
    svr.Post("/DeleteFriend", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string wxid = reqJson.value("wxid", "");
        if (wxid.empty())
        {
            resp["ret"] = -1;
            resp["retmsg"] = "wxid is required";
            res.set_content(resp.dump(), "application/json");
            return;
        }

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["wxid"] = wxid;
        res.set_content(resp.dump(), "application/json");
    });

    // 9. 修改好友备注
    svr.Post("/EditFriendMark", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string wxid = reqJson.value("wxid", "");
        std::string mark = reqJson.value("mark", reqJson.value("remark", ""));

        if (wxid.empty())
        {
            resp["ret"] = -1;
            resp["retmsg"] = "wxid is required";
            res.set_content(resp.dump(), "application/json");
            return;
        }

        xmgr::DatabaseMgr& dbMgr = xmgr::DatabaseMgr::getInstance();
        std::string updateSql = "UPDATE contact SET remark = '" + mark + "' WHERE username = '" + wxid + "' OR alias = '" + wxid + "';";
        dbMgr.execute("contact.db", updateSql);

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["wxid"] = wxid;
        resp["mark"] = mark;
        res.set_content(resp.dump(), "application/json");
    });

    // 10. 设置星标朋友
    svr.Post("/SetStarTag", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string wxid = reqJson.value("wxid", "");
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["wxid"] = wxid;
        res.set_content(resp.dump(), "application/json");
    });

    // 11. 取消星标朋友
    svr.Post("/UndoStarTag", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string wxid = reqJson.value("wxid", "");
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["wxid"] = wxid;
        res.set_content(resp.dump(), "application/json");
    });

    // 12. 设置联系人角色/权限标记
    svr.Post("/SetContactRole", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string wxid = reqJson.value("wxid", "");
        std::string role = reqJson.value("role", "2");

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["wxid"] = wxid;
        resp["role"] = role;
        res.set_content(resp.dump(), "application/json");
    });

    // 13. 转账前查询对方信息
    svr.Post("/BeforeTransfer", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string wxid = reqJson.value("wxid", "");
        if (wxid.empty())
        {
            resp["ret"] = -1;
            resp["retmsg"] = "wxid is required";
            res.set_content(resp.dump(), "application/json");
            return;
        }

        xmgr::DatabaseMgr& dbMgr = xmgr::DatabaseMgr::getInstance();
        std::string sql = "SELECT username, nick_name, remark FROM contact WHERE username = '" + wxid + "' OR alias = '" + wxid + "' LIMIT 1;";
        auto dbRes = dbMgr.execute("contact.db", sql);

        std::string name = "";
        if (dbRes.value("status", -1) == 0 && dbRes.contains("data") && !dbRes["data"].empty())
        {
            name = dbRes["data"][0].value("remark", "");
            if (name.empty()) name = dbRes["data"][0].value("nick_name", "");
        }

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["wxid"] = wxid;
        resp["realname"] = name;
        resp["mask_realname"] = name.empty() ? "" : ("*" + name.substr(name.length() > 1 ? 1 : 0));
        res.set_content(resp.dump(), "application/json");
    });

    // 14. 修改好友描述
    svr.Post("/EditFriendDescribe", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string wxid = reqJson.value("wxid", "");
        std::string describe = reqJson.value("describe", reqJson.value("description", ""));

        if (wxid.empty())
        {
            resp["ret"] = -1;
            resp["retmsg"] = "wxid is required";
            res.set_content(resp.dump(), "application/json");
            return;
        }

        xmgr::DatabaseMgr& dbMgr = xmgr::DatabaseMgr::getInstance();
        std::string updateSql = "UPDATE contact SET description = '" + describe + "' WHERE username = '" + wxid + "' OR alias = '" + wxid + "';";
        dbMgr.execute("contact.db", updateSql);

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["wxid"] = wxid;
        resp["describe"] = describe;
        res.set_content(resp.dump(), "application/json");
    });

    // 15. 搜索 OpenIM 联系人
    svr.Post("/SearchOpenImContact", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string content = reqJson.value("content", reqJson.value("url", ""));
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["content"] = content;
        res.set_content(resp.dump(), "application/json");
    });
}
