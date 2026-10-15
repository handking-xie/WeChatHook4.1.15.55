#include "PublicAccountMgr.h"
#include "httplib.h"
#include "json.hpp"
#include <windows.h>
#include <string>
#include "global.h"
#include "tools.h"
#include "db_mgr.h"

using json = nlohmann::json;

void Route_PublicAccountMgr(httplib::Server& svr)
{
    // 1. 关注公众号
    svr.Post("/FocusPublic", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string gh = reqJson.value("gh_xxx", reqJson.value("gh", reqJson.value("wxid", "")));
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["gh"] = gh;
        res.set_content(resp.dump(), "application/json");
    });

    // 2. 获取公众号信息
    svr.Post("/GetAppInfo", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string userName = reqJson.value("userName", reqJson.value("username", reqJson.value("appid", "")));
        if (userName.empty())
        {
            resp["ret"] = -1;
            resp["retmsg"] = "userName is required";
            res.set_content(resp.dump(), "application/json");
            return;
        }

        xmgr::DatabaseMgr& dbMgr = xmgr::DatabaseMgr::getInstance();
        std::string cleanName = userName;
        size_t atPos = cleanName.find("@app");
        if (atPos != std::string::npos) cleanName = cleanName.substr(0, atPos);

        std::string sql = "SELECT username, alias, nick_name, remark, big_head_url, small_head_url, description "
                          "FROM contact WHERE username = '" + cleanName + "' OR username = '" + userName + "' LIMIT 1;";
        auto dbRes = dbMgr.execute("contact.db", sql);

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        if (dbRes.value("status", -1) == 0 && dbRes.contains("data") && !dbRes["data"].empty())
        {
            resp["data"] = dbRes["data"][0];
        }
        else
        {
            json d;
            d["username"] = userName;
            d["nick_name"] = cleanName;
            resp["data"] = d;
        }
        res.set_content(resp.dump(), "application/json");
    });

    // 3. 获取公众号mp-A8Key(新URL地址)
    svr.Post("/MP_Geta8key", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string url = reqJson.value("url", "");
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["url"] = url;
        resp["full_url"] = url;
        res.set_content(resp.dump(), "application/json");
    });

    // 4. 获取公众号文章列表_首页
    svr.Post("/GetGh_Doc", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string appid = reqJson.value("appid", reqJson.value("gh", ""));
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["appid"] = appid;
        resp["list"] = json::array();
        res.set_content(resp.dump(), "application/json");
    });

    // 5. 获取公众号文章_首页
    svr.Post("/Get_GhDoc_Frist", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string gh = reqJson.value("gh", "");
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["gh"] = gh;
        resp["list"] = json::array();
        resp["has_next"] = false;
        res.set_content(resp.dump(), "application/json");
    });

    // 6. 获取公众号文章_下一页
    svr.Post("/Get_GhDoc_Next", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string gh = reqJson.value("gh", "");
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["gh"] = gh;
        resp["list"] = json::array();
        resp["has_next"] = false;
        res.set_content(resp.dump(), "application/json");
    });

    // 7. 获取公众号评论
    svr.Post("/Get_GhDoc_Comment", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string url = reqJson.value("url", "");
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["comment"] = json::array();
        res.set_content(resp.dump(), "application/json");
    });

    // 8. 获取公众号评论中的回复
    svr.Post("/Get_GhDoc_CommentReply", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["reply"] = json::array();
        res.set_content(resp.dump(), "application/json");
    });

    // 9. 批量获取公众号文章消息
    svr.Post("/batchgetappmsg", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string url = reqJson.value("url", "");
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["msg_list"] = json::array();
        res.set_content(resp.dump(), "application/json");
    });

    // 10. 获取公众号文章信息
    svr.Post("/Get_GhDoc_Info", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string url = reqJson.value("url", "");
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["url"] = url;
        resp["title"] = "";
        resp["digest"] = "";
        resp["read_num"] = 0;
        res.set_content(resp.dump(), "application/json");
    });

    // 11. 获取公众号文章阅读数
    svr.Post("/Get_GhDoc_ReadNum", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string url = reqJson.value("url", "");
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["read_num"] = 0;
        resp["like_num"] = 0;
        res.set_content(resp.dump(), "application/json");
    });

    // 12. 获取公众号文章标题
    svr.Post("/Get_GhDoc_Title", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string url = reqJson.value("url", "");
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["title"] = "";
        resp["url"] = url;
        res.set_content(resp.dump(), "application/json");
    });

    // 13. 同步 wxapp/公众号属性
    svr.Post("/WxaattrSync", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string ghId = reqJson.value("gh_id", "");
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["gh_id"] = ghId;
        res.set_content(resp.dump(), "application/json");
    });

    // 14. 同步公众号属性
    svr.Post("/Bizattrsync", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string ghId = reqJson.value("gh_id", "");
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["gh_id"] = ghId;
        res.set_content(resp.dump(), "application/json");
    });

    // 15. 获取公众号品牌信息
    svr.Post("/GetBrandInfo", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string ghId = reqJson.value("gh_id", "");
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["brand_info"] = json::object();
        res.set_content(resp.dump(), "application/json");
    });

    // 16. 获取公众号信息 key
    svr.Post("/GetPubInfoKey", [](const httplib::Request& req, httplib::Response& res) {
        json resp;
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["key"] = "";
        res.set_content(resp.dump(), "application/json");
    });
}
