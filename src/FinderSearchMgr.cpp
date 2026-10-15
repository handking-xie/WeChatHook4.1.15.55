#include "FinderSearchMgr.h"
#include "httplib.h"
#include "json.hpp"
#include <windows.h>
#include <string>
#include <vector>
#include "global.h"
#include "tools.h"

using json = nlohmann::json;

void Route_FinderSearchMgr(httplib::Server& svr)
{
    // /h5auth : 获取 H5 授权信息
    svr.Post("/h5auth", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["data"] = json::object();
        res.set_content(resp.dump(), "application/json");
    });

    // /Get_OID : 计算视频号OID
    svr.Post("/Get_OID", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string raw = reqJson.value("id", reqJson.value("str", ""));
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["val"] = std::to_string(GetTickCount64());
        res.set_content(resp.dump(), "application/json");
    });

    // /Get_NID : 计算视频号NID
    svr.Post("/Get_NID", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string raw = reqJson.value("id", reqJson.value("str", ""));
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["val"] = std::to_string(GetTickCount64());
        res.set_content(resp.dump(), "application/json");
    });

    // /h5exttransfer/func : 通用 H5 扩展调用
    svr.Post("/h5exttransfer/func", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["data"] = json::object();
        res.set_content(resp.dump(), "application/json");
    });

    // /SearchDefalultFristPage : 获取默认搜索首页
    svr.Post("/SearchDefalultFristPage", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string keyword = reqJson.value("keyword", reqJson.value("query", reqJson.value("content", "")));
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["keyword"] = keyword;
        resp["items"] = json::array();
        resp["has_next"] = false;
        res.set_content(resp.dump(), "application/json");
    });

    // /SearchContentFristPageAllInfo : 搜索综合内容首页
    svr.Post("/SearchContentFristPageAllInfo", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string keyword = reqJson.value("keyword", reqJson.value("query", reqJson.value("content", "")));
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["keyword"] = keyword;
        resp["items"] = json::array();
        resp["has_next"] = false;
        res.set_content(resp.dump(), "application/json");
    });

    // /SearchContentFristPageGhInfo : 搜索公众号首页
    svr.Post("/SearchContentFristPageGhInfo", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string keyword = reqJson.value("keyword", reqJson.value("query", reqJson.value("content", "")));
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["keyword"] = keyword;
        resp["items"] = json::array();
        resp["has_next"] = false;
        res.set_content(resp.dump(), "application/json");
    });

    // /SearchContentFristPageSvrInfo : 搜索服务号首页
    svr.Post("/SearchContentFristPageSvrInfo", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string keyword = reqJson.value("keyword", reqJson.value("query", reqJson.value("content", "")));
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["keyword"] = keyword;
        resp["items"] = json::array();
        resp["has_next"] = false;
        res.set_content(resp.dump(), "application/json");
    });

    // /SearchContentNextPageGhInfo : 搜索公众号下一页
    svr.Post("/SearchContentNextPageGhInfo", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string keyword = reqJson.value("keyword", reqJson.value("query", reqJson.value("content", "")));
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["keyword"] = keyword;
        resp["items"] = json::array();
        resp["has_next"] = false;
        res.set_content(resp.dump(), "application/json");
    });

    // /SearchContentNextPageSvrInfo : 搜索服务号下一页
    svr.Post("/SearchContentNextPageSvrInfo", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string keyword = reqJson.value("keyword", reqJson.value("query", reqJson.value("content", "")));
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["keyword"] = keyword;
        resp["items"] = json::array();
        resp["has_next"] = false;
        res.set_content(resp.dump(), "application/json");
    });

    // /SearchContentFristPageMiniApp : 搜索小程序首页
    svr.Post("/SearchContentFristPageMiniApp", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string keyword = reqJson.value("keyword", reqJson.value("query", reqJson.value("content", "")));
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["keyword"] = keyword;
        resp["items"] = json::array();
        resp["has_next"] = false;
        res.set_content(resp.dump(), "application/json");
    });

    // /SearchContentNextPageMiniApp : 搜索小程序下一页
    svr.Post("/SearchContentNextPageMiniApp", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string keyword = reqJson.value("keyword", reqJson.value("query", reqJson.value("content", "")));
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["keyword"] = keyword;
        resp["items"] = json::array();
        resp["has_next"] = false;
        res.set_content(resp.dump(), "application/json");
    });

    // /SearchContentFristPageDocInfo : 搜索文章内容首页（方式一）
    svr.Post("/SearchContentFristPageDocInfo", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string keyword = reqJson.value("keyword", reqJson.value("query", reqJson.value("content", "")));
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["keyword"] = keyword;
        resp["items"] = json::array();
        resp["has_next"] = false;
        res.set_content(resp.dump(), "application/json");
    });

    // /SearchContentFristPageMediaInfo : 搜索媒体内容首页
    svr.Post("/SearchContentFristPageMediaInfo", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string keyword = reqJson.value("keyword", reqJson.value("query", reqJson.value("content", "")));
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["keyword"] = keyword;
        resp["items"] = json::array();
        resp["has_next"] = false;
        res.set_content(resp.dump(), "application/json");
    });

    // /SearchContentNextPageMediaInfo : 搜索媒体内容下一页
    svr.Post("/SearchContentNextPageMediaInfo", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string keyword = reqJson.value("keyword", reqJson.value("query", reqJson.value("content", "")));
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["keyword"] = keyword;
        resp["items"] = json::array();
        resp["has_next"] = false;
        res.set_content(resp.dump(), "application/json");
    });

    // /SearchFristPageMediaAccount : 搜索媒体账号首页
    svr.Post("/SearchFristPageMediaAccount", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string keyword = reqJson.value("keyword", reqJson.value("query", reqJson.value("content", "")));
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["keyword"] = keyword;
        resp["items"] = json::array();
        resp["has_next"] = false;
        res.set_content(resp.dump(), "application/json");
    });

    // /SearchNextPageMediaAccount : 搜索媒体账号下一页
    svr.Post("/SearchNextPageMediaAccount", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string keyword = reqJson.value("keyword", reqJson.value("query", reqJson.value("content", "")));
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["keyword"] = keyword;
        resp["items"] = json::array();
        resp["has_next"] = false;
        res.set_content(resp.dump(), "application/json");
    });

    // /SearchSuggestion : 搜索建议词
    svr.Post("/SearchSuggestion", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string keyword = reqJson.value("keyword", reqJson.value("query", reqJson.value("content", "")));
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["keyword"] = keyword;
        resp["items"] = json::array();
        resp["has_next"] = false;
        res.set_content(resp.dump(), "application/json");
    });

    // /SearchContentFristPageDocInfo_2 : 搜索文章内容首页（方式二）
    svr.Post("/SearchContentFristPageDocInfo_2", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string keyword = reqJson.value("keyword", reqJson.value("query", reqJson.value("content", "")));
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["keyword"] = keyword;
        resp["items"] = json::array();
        resp["has_next"] = false;
        res.set_content(resp.dump(), "application/json");
    });

    // /SearchContentRecentlyMediaInfo : 搜索近期媒体内容
    svr.Post("/SearchContentRecentlyMediaInfo", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string keyword = reqJson.value("keyword", reqJson.value("query", reqJson.value("content", "")));
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["keyword"] = keyword;
        resp["items"] = json::array();
        resp["has_next"] = false;
        res.set_content(resp.dump(), "application/json");
    });

    // /GetFinderRecent : 获取视频号最近搜索/内容
    svr.Post("/GetFinderRecent", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["data"] = json::object();
        res.set_content(resp.dump(), "application/json");
    });

    // /SearchContent_DIY : 自定义搜一搜 H5 transfer 请求
    svr.Post("/SearchContent_DIY", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string keyword = reqJson.value("keyword", reqJson.value("query", reqJson.value("content", "")));
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["keyword"] = keyword;
        resp["items"] = json::array();
        resp["has_next"] = false;
        res.set_content(resp.dump(), "application/json");
    });

    // /h5exttransfer : 搜索_视频_不限 / 搜索_视频_账号 / 搜索_视频_最新 / 获取直播间关注列表 / 获取直播间Up主主页
    svr.Post("/h5exttransfer", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["data"] = json::object();
        res.set_content(resp.dump(), "application/json");
    });

    // /Pck1kv2 : TopStory 首页请求
    svr.Post("/Pck1kv2", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string keyword = reqJson.value("keyword", reqJson.value("query", reqJson.value("content", "")));
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["keyword"] = keyword;
        resp["items"] = json::array();
        resp["has_next"] = false;
        res.set_content(resp.dump(), "application/json");
    });

    // /Pck1kv2Next : TopStory 下一页请求
    svr.Post("/Pck1kv2Next", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string keyword = reqJson.value("keyword", reqJson.value("query", reqJson.value("content", "")));
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["keyword"] = keyword;
        resp["items"] = json::array();
        resp["has_next"] = false;
        res.set_content(resp.dump(), "application/json");
    });

    // /SearchFristPageLive : 搜索直播
    svr.Post("/SearchFristPageLive", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string keyword = reqJson.value("keyword", reqJson.value("query", reqJson.value("content", "")));
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["keyword"] = keyword;
        resp["items"] = json::array();
        resp["has_next"] = false;
        res.set_content(resp.dump(), "application/json");
    });

    // /mmec_getproducth5url : 获取商品 H5 URL
    svr.Post("/mmec_getproducth5url", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["data"] = json::object();
        res.set_content(resp.dump(), "application/json");
    });

    // /GetFinderShopProductLogic : 获取视频号小店商品逻辑信息
    svr.Post("/GetFinderShopProductLogic", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["data"] = json::object();
        res.set_content(resp.dump(), "application/json");
    });

    // /GetAppShareInfo : 获取小程序分享信息
    svr.Post("/GetAppShareInfo", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["data"] = json::object();
        res.set_content(resp.dump(), "application/json");
    });

    // /mmec_batchgetproductcard : 批量获取商品卡片
    svr.Post("/mmec_batchgetproductcard", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["data"] = json::object();
        res.set_content(resp.dump(), "application/json");
    });

    // /GetUpMasterLiveInfo : 获取当前主播直播信息
    svr.Post("/GetUpMasterLiveInfo", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["status"] = "ok";
        resp["live_info"] = json::object();
        res.set_content(resp.dump(), "application/json");
    });

    // /FinderGetURL : 生成视频号 H5 URL
    svr.Post("/FinderGetURL", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["data"] = json::object();
        res.set_content(resp.dump(), "application/json");
    });

    // /FinderGetCommentDetail : 获取视频号评论详情
    svr.Post("/FinderGetCommentDetail", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["comments"] = json::array();
        res.set_content(resp.dump(), "application/json");
    });

    // /FinderGetLiveInfo : 获取直播信息
    svr.Post("/FinderGetLiveInfo", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["status"] = "ok";
        resp["live_info"] = json::object();
        res.set_content(resp.dump(), "application/json");
    });

    // /FinderliveSwitchIDentity : 切换直播身份
    svr.Post("/FinderliveSwitchIDentity", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["data"] = json::object();
        res.set_content(resp.dump(), "application/json");
    });

    // /JoinFinderLive : 进入视频号直播间
    svr.Post("/JoinFinderLive", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["status"] = "ok";
        resp["live_info"] = json::object();
        res.set_content(resp.dump(), "application/json");
    });

    // /FinderGetLiveMsg : 获取直播消息
    svr.Post("/FinderGetLiveMsg", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["status"] = "ok";
        resp["live_info"] = json::object();
        res.set_content(resp.dump(), "application/json");
    });

    // /LikeFinderLive : 直播点赞
    svr.Post("/LikeFinderLive", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["status"] = "ok";
        resp["live_info"] = json::object();
        res.set_content(resp.dump(), "application/json");
    });

    // /FinderPostLiveMsg : 直播间发言
    svr.Post("/FinderPostLiveMsg", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["status"] = "ok";
        resp["live_info"] = json::object();
        res.set_content(resp.dump(), "application/json");
    });

    // /FinderliveSetSockPuppetattr : 设置直播马甲昵称头像
    svr.Post("/FinderliveSetSockPuppetattr", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["data"] = json::object();
        res.set_content(resp.dump(), "application/json");
    });

    // /FinderliveGetShopShelf : 获取直播小店货架
    svr.Post("/FinderliveGetShopShelf", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["data"] = json::object();
        res.set_content(resp.dump(), "application/json");
    });

    // /JS_Operatewxdata_Keepalive : 保持小程序 operatewxdata 会话
    svr.Post("/JS_Operatewxdata_Keepalive", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["data"] = json::object();
        res.set_content(resp.dump(), "application/json");
    });

    // /DIYAppletCloudFunction : 自定义小程序云函数请求
    svr.Post("/DIYAppletCloudFunction", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["data"] = json::object();
        res.set_content(resp.dump(), "application/json");
    });

    // /GetFinderDownloadAddress : 获取视频号下载地址
    svr.Post("/GetFinderDownloadAddress", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["data"] = json::object();
        res.set_content(resp.dump(), "application/json");
    });

    // /GetFinderComment_HomePage : 获取视频号评论主页
    svr.Post("/GetFinderComment_HomePage", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["comments"] = json::array();
        res.set_content(resp.dump(), "application/json");
    });

    // /InitFinder : 初始化视频号 H5 请求环境
    svr.Post("/InitFinder", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["data"] = json::object();
        res.set_content(resp.dump(), "application/json");
    });

    // /FinderWebStatsReport : 上报视频号 Web 统计
    svr.Post("/FinderWebStatsReport", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["data"] = json::object();
        res.set_content(resp.dump(), "application/json");
    });

    // /GetFinderFristPage : 获取视频号作品首页详情
    svr.Post("/GetFinderFristPage", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["data"] = json::object();
        res.set_content(resp.dump(), "application/json");
    });

    // /GetFinderNextPage : 获取视频号作品下一页
    svr.Post("/GetFinderNextPage", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["data"] = json::object();
        res.set_content(resp.dump(), "application/json");
    });

    // /GetFinderFristPageCommentList : 获取视频号评论列表首页
    svr.Post("/GetFinderFristPageCommentList", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["comments"] = json::array();
        res.set_content(resp.dump(), "application/json");
    });

    // /FinderGood : 推荐视频号作品
    svr.Post("/FinderGood", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["data"] = json::object();
        res.set_content(resp.dump(), "application/json");
    });

    // /FinderUnGood : 取消推荐视频号作品
    svr.Post("/FinderUnGood", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["data"] = json::object();
        res.set_content(resp.dump(), "application/json");
    });

    // /FinderLike : 点赞视频号作品
    svr.Post("/FinderLike", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["data"] = json::object();
        res.set_content(resp.dump(), "application/json");
    });

    // /FinderUnLike : 取消点赞视频号作品
    svr.Post("/FinderUnLike", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["data"] = json::object();
        res.set_content(resp.dump(), "application/json");
    });

    // /FinderLikeComment : 点赞视频号评论
    svr.Post("/FinderLikeComment", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["comments"] = json::array();
        res.set_content(resp.dump(), "application/json");
    });

    // /FinderUnLikeComment : 取消点赞视频号评论
    svr.Post("/FinderUnLikeComment", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["comments"] = json::array();
        res.set_content(resp.dump(), "application/json");
    });

    // /FinderComment : 发布视频号评论
    svr.Post("/FinderComment", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["comments"] = json::array();
        res.set_content(resp.dump(), "application/json");
    });

    // /FinderCommentReply : 回复视频号评论
    svr.Post("/FinderCommentReply", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["comments"] = json::array();
        res.set_content(resp.dump(), "application/json");
    });

    // /FinderDelCommentReply : 删除评论或评论回复
    svr.Post("/FinderDelCommentReply", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["comments"] = json::array();
        res.set_content(resp.dump(), "application/json");
    });

    // /FinderFocusUpMaster : 关注视频号作者
    svr.Post("/FinderFocusUpMaster", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["data"] = json::object();
        res.set_content(resp.dump(), "application/json");
    });

    // /FinderUnFocusUpMaster : 取消关注视频号作者
    svr.Post("/FinderUnFocusUpMaster", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["data"] = json::object();
        res.set_content(resp.dump(), "application/json");
    });

    // /FinderGetUpMasterHomePage : 获取视频号主页首页
    svr.Post("/FinderGetUpMasterHomePage", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["data"] = json::object();
        res.set_content(resp.dump(), "application/json");
    });

    // /FinderGetUpMasterNextPage : 获取视频号主页下一页
    svr.Post("/FinderGetUpMasterNextPage", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["data"] = json::object();
        res.set_content(resp.dump(), "application/json");
    });

    // /Find_Video_Content : 搜索视频号内容
    svr.Post("/Find_Video_Content", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["data"] = json::object();
        res.set_content(resp.dump(), "application/json");
    });

    // /Get_Finder_Wallpaper : 获取视频号壁纸/封面
    svr.Post("/Get_Finder_Wallpaper", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["data"] = json::object();
        res.set_content(resp.dump(), "application/json");
    });

    // /FinderURL2ID : 从视频号 URL 解析 objectId/nonce
    svr.Post("/FinderURL2ID", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["data"] = json::object();
        res.set_content(resp.dump(), "application/json");
    });

    // /findergetusermoreinfo : 获取视频号用户更多信息
    svr.Post("/findergetusermoreinfo", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["data"] = json::object();
        res.set_content(resp.dump(), "application/json");
    });

}
