#include "MiniAppMgr.h"
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

void Route_MiniAppMgr(httplib::Server& svr)
{
    // 1. App OAuth A8Key
    svr.Post("/AppOAuth", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string appid = reqJson.value("appid", "");
        std::string url = reqJson.value("url", "");

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["appid"] = appid;
        resp["url"] = url;
        resp["full_url"] = url;
        res.set_content(resp.dump(), "application/json");
    });

    // 2. 第三方 OAuth A8Key
    svr.Post("/OAuth", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string appid = reqJson.value("appid", "");
        std::string url = reqJson.value("url", "");

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["appid"] = appid;
        resp["url"] = url;
        res.set_content(resp.dump(), "application/json");
    });

    // 3. 登陆小程序/获取小程序code
    svr.Post("/GetMiniAppCode", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string appid = reqJson.value("appid", "");
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["appid"] = appid;
        resp["code"] = "0" + std::to_string(GetTickCount64()) + "mock_code";
        res.set_content(resp.dump(), "application/json");
    });

    // 4. 获取小程序 openid
    svr.Post("/Get_Applet_Openid", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string appid = reqJson.value("appid", "");
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["appid"] = appid;
        resp["openid"] = "oWx" + std::to_string(GetTickCount64());
        res.set_content(resp.dump(), "application/json");
    });

    // 5. 刷新小程序 session
    svr.Post("/Applet_RefreshSession", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string appid = reqJson.value("appid", "");
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["appid"] = appid;
        res.set_content(resp.dump(), "application/json");
    });

    // 6. 小程序订阅相关请求
    svr.Post("/Applet_Subscribe", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string ghAccount = reqJson.value("gh_account", "");
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["gh_account"] = ghAccount;
        res.set_content(resp.dump(), "application/json");
    });

    // 7. 获取本地小程序 session id
    svr.Post("/GetMiniAppSession", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string appid = reqJson.value("appid", "");
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["appid"] = appid;
        resp["session_id"] = "sess_" + std::to_string(GetTickCount64());
        res.set_content(resp.dump(), "application/json");
    });

    // 8. 获取小程序访问资料
    svr.Post("/GetAppAccessInfo", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string appid = reqJson.value("appid", "");
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["appid"] = appid;
        resp["access_token"] = "";
        res.set_content(resp.dump(), "application/json");
    });

    // 9. 获取小程序支付二维码
    svr.Post("/GetPayQRCode", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string appid = reqJson.value("appid", "");
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["appid"] = appid;
        resp["qrcode_url"] = "https://payapp.weixin.qq.com/pay_qr/" + appid;
        res.set_content(resp.dump(), "application/json");
    });

    // 10. 启动小程序
    svr.Post("/Launchwxapp", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string appid = reqJson.value("appid", "");
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["appid"] = appid;
        resp["status"] = "launched";
        res.set_content(resp.dump(), "application/json");
    });

    // 11. 获取账本/账单数据
    svr.Post("/GetLedger", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["records"] = json::array();
        res.set_content(resp.dump(), "application/json");
    });

    // 12. 获取小程序使用记录
    svr.Post("/GetWxAppUsageRecord", [](const httplib::Request& req, httplib::Response& res) {
        json resp;
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["record_list"] = json::array();
        res.set_content(resp.dump(), "application/json");
    });

    // 13. 批量获取小程序/应用信息
    svr.Post("/BatchGetAppInfo", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string ghList = reqJson.value("gh_idlist", "");
        json arr = json::array();

        if (!ghList.empty())
        {
            std::stringstream ss(ghList);
            std::string item;
            xmgr::DatabaseMgr& dbMgr = xmgr::DatabaseMgr::getInstance();

            while (std::getline(ss, item, ','))
            {
                size_t s = item.find_first_not_of(" \t\r\n");
                size_t e = item.find_last_not_of(" \t\r\n");
                if (s != std::string::npos && e != std::string::npos)
                {
                    std::string u = item.substr(s, e - s + 1);
                    std::string clean = u;
                    size_t p = clean.find("@app");
                    if (p != std::string::npos) clean = clean.substr(0, p);

                    std::string sql = "SELECT username, nick_name, small_head_url FROM contact WHERE username = '" + clean + "' OR username = '" + u + "' LIMIT 1;";
                    auto dbRes = dbMgr.execute("contact.db", sql);

                    json o;
                    o["app_id"] = u;
                    if (dbRes.value("status", -1) == 0 && dbRes.contains("data") && !dbRes["data"].empty())
                    {
                        o["app_name"] = dbRes["data"][0].value("nick_name", clean);
                        o["icon"] = dbRes["data"][0].value("small_head_url", "");
                    }
                    else
                    {
                        o["app_name"] = clean;
                        o["icon"] = "";
                    }
                    arr.push_back(o);
                }
            }
        }

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["apps"] = arr;
        res.set_content(resp.dump(), "application/json");
    });

    // 14. 获取小程序评论
    svr.Post("/GetAppletComment", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string appid = reqJson.value("appid", "");
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["appid"] = appid;
        resp["comments"] = json::array();
        res.set_content(resp.dump(), "application/json");
    });

    // 15. 获取 IP 定位信息
    svr.Post("/GetLocationInfoByIP", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string appid = reqJson.value("appid", "");
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["appid"] = appid;
        resp["country"] = "CN";
        resp["province"] = "Guangdong";
        resp["city"] = "Shenzhen";
        res.set_content(resp.dump(), "application/json");
    });

    // 16. 异步获取小程序码
    svr.Post("/GetMiniAppCode_Async", [](const httplib::Request& req, httplib::Response& res) {
        json resp;
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["task_id"] = std::to_string(GetTickCount64());
        res.set_content(resp.dump(), "application/json");
    });
}
