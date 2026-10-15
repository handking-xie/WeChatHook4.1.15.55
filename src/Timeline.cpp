#include "httplib.h"
#include "json.hpp"
#include <windows.h>
#include <string>
#include <algorithm>
#include "global.h"
#include "tools.h"
#include "db_mgr.h"

#include "Timeline.h"
#include "TimelineRequest.h"
#include "FriendTimelineRoute.h"
#include "SnsLikeRoute.h"

using json = nlohmann::json;

/* ========================================================
   辅助函数：从朋友圈 XML 中提取指定标签内容
   ======================================================== */
static std::string ExtractXmlTag(const std::string& xml, const std::string& tag)
{
    std::string openTag = "<" + tag + ">";
    std::string closeTag = "</" + tag + ">";
    size_t start = xml.find(openTag);
    if (start == std::string::npos) return "";
    start += openTag.length();
    size_t end = xml.find(closeTag, start);
    if (end == std::string::npos) return "";
    return xml.substr(start, end - start);
}

/* ========================================================
   核心处理：读取朋友圈首页与下一页数据
   ======================================================== */
static void HandleGetTimeline(const httplib::Request& req, httplib::Response& res, bool isNextPage, bool requireUser = false)
{
    json resp;
    timeline_request::Parameters parameters;
    std::string error;
    if (!timeline_request::Parse(req.body, requireUser, parameters, error))
    {
        resp["ret"] = -1;
        resp["retmsg"] = error;
        resp["data"] = json::array();
        res.set_content(resp.dump(), "application/json");
        return;
    }
    const int count = parameters.count;
    const std::string nextId = timeline_request::Escape(parameters.cursor);
    const std::string xmlCursor = timeline_request::EscapeLike(parameters.cursor);
    const std::string targetUser = timeline_request::Escape(parameters.user);
    xmgr::DatabaseMgr& dbMgr = xmgr::DatabaseMgr::getInstance();
    if (dbMgr.getDatabaseHandle("sns.db") == nullptr)
    {
        resp["ret"] = -1;
        resp["retmsg"] = "sns.db is not loaded or user not logged in";
        resp["data"] = json::array();
        res.set_content(resp.dump(), "application/json");
        return;
    }

    std::string sql;
    if (targetUser.empty())
    {
        // 1. “发现 -> 朋友圈” 好友时间线（SnsMainTimeLineBreakFlag）
        if (isNextPage && !nextId.empty())
        {
            sql = "WITH cur AS ("
                  "  SELECT tid_heigh_bit, tid_low_bit FROM SnsMainTimeLineBreakFlag "
                  "  WHERE tid = '" + nextId + "' "
                  "     OR tid = (SELECT tid FROM SnsTimeLine WHERE content LIKE '%<id>" + xmlCursor + "</id>%' ESCAPE '\\' LIMIT 1) "
                  "  LIMIT 1"
                  ") "
                  "SELECT m.tid, m.tid_heigh_bit, m.tid_low_bit, s.user_name, s.content "
                  "FROM SnsMainTimeLineBreakFlag m "
                  "INNER JOIN SnsTimeLine s ON m.tid = s.tid "
                  "JOIN cur c "
                  "WHERE (m.tid_heigh_bit < c.tid_heigh_bit) "
                  "   OR (m.tid_heigh_bit = c.tid_heigh_bit AND m.tid_low_bit < c.tid_low_bit) "
                  "ORDER BY m.tid_heigh_bit DESC, m.tid_low_bit DESC "
                  "LIMIT " + std::to_string(count);
        }
        else
        {
            sql = "SELECT m.tid, m.tid_heigh_bit, m.tid_low_bit, s.user_name, s.content "
                  "FROM SnsMainTimeLineBreakFlag m "
                  "INNER JOIN SnsTimeLine s ON m.tid = s.tid "
                  "ORDER BY m.tid_heigh_bit DESC, m.tid_low_bit DESC "
                  "LIMIT " + std::to_string(count);
        }
    }
    else
    {
        // 2. 指定个人相册朋友圈（SnsUserTimeLineBreakFlagV2）
        if (isNextPage && !nextId.empty())
        {
            sql = "WITH cur AS ("
                  "  SELECT tid_heigh_bit, tid_low_bit FROM SnsUserTimeLineBreakFlagV2 "
                  "  WHERE user_name = '" + targetUser + "' AND ("
                  "     tid = '" + nextId + "' "
                  "     OR tid = (SELECT tid FROM SnsTimeLine WHERE user_name = '" + targetUser + "' AND content LIKE '%<id>" + xmlCursor + "</id>%' ESCAPE '\\' LIMIT 1)"
                  "  ) "
                  "  LIMIT 1"
                  ") "
                  "SELECT m.tid, m.tid_heigh_bit, m.tid_low_bit, s.user_name, s.content "
                  "FROM SnsUserTimeLineBreakFlagV2 m "
                  "INNER JOIN SnsTimeLine s ON m.tid = s.tid "
                  "JOIN cur c "
                  "WHERE m.user_name = '" + targetUser + "' "
                  "  AND ((m.tid_heigh_bit < c.tid_heigh_bit) "
                  "       OR (m.tid_heigh_bit = c.tid_heigh_bit AND m.tid_low_bit < c.tid_low_bit)) "
                  "ORDER BY m.tid_heigh_bit DESC, m.tid_low_bit DESC "
                  "LIMIT " + std::to_string(count);
        }
        else
        {
            sql = "SELECT m.tid, m.tid_heigh_bit, m.tid_low_bit, s.user_name, s.content "
                  "FROM SnsUserTimeLineBreakFlagV2 m "
                  "INNER JOIN SnsTimeLine s ON m.tid = s.tid "
                  "WHERE m.user_name = '" + targetUser + "' "
                  "ORDER BY m.tid_heigh_bit DESC, m.tid_low_bit DESC "
                  "LIMIT " + std::to_string(count);
        }
    }

    auto dbRes = dbMgr.execute("sns.db", sql);
    if (dbRes.value("status", -1) != 0 || !dbRes.contains("data") || !dbRes["data"].is_array())
    {
        resp["ret"] = -1;
        resp["retmsg"] = dbRes.value("desc", "query sns.db failed");
        resp["data"] = json::array();
        res.set_content(resp.dump(), "application/json");
        return;
    }

    json items = json::array();
    std::string lastId;

    for (auto& row : dbRes["data"])
    {
        json item;
        std::string tid = row.value("tid", "");
        std::string contentXml = row.value("content", "");
        std::string userName = row.value("user_name", "");

        item["tid"] = tid;
        item["user_name"] = userName;
        item["content"] = contentXml;

        // 提取 XML 中的常用核心字段
        if (!contentXml.empty())
        {
            std::string xmlId = ExtractXmlTag(contentXml, "id");
            std::string createTime = ExtractXmlTag(contentXml, "createTime");
            std::string contentDesc = ExtractXmlTag(contentXml, "contentDesc");
            std::string nickname = ExtractXmlTag(contentXml, "nickname");

            if (!xmlId.empty()) item["sns_id"] = xmlId;
            if (!createTime.empty()) item["create_time"] = createTime;
            if (!contentDesc.empty()) item["content_desc"] = contentDesc;
            if (!nickname.empty()) item["nickname"] = nickname;

            if (!xmlId.empty()) lastId = xmlId;
            else if (!tid.empty()) lastId = tid;
        }
        else
        {
            if (!tid.empty()) lastId = tid;
        }

        items.push_back(item);
    }

    // 仅调整本页返回顺序；保留按原始查询顺序生成的分页游标。
    auto createTimeValue = [](const json& item) -> long long {
        try
        {
            return std::stoll(item.value("create_time", "0"));
        }
        catch (...)
        {
            return 0;
        }
    };
    std::stable_sort(items.begin(), items.end(), [&](const json& a, const json& b) {
        return createTimeValue(a) > createTimeValue(b);
    });

    resp["ret"] = 0;
    resp["retmsg"] = "success";
    resp["count"] = items.size();
    if (requireUser) {
        resp["source"] = "local_cache";
        resp["user_name"] = parameters.user;
    }
    if (!lastId.empty())
    {
        resp["next_id"] = lastId;
    }
    resp["data"] = items;

    res.set_content(resp.dump(), "application/json");
}

/* ========================================================
   路由注册
   ======================================================== */
void Route_Timeline(httplib::Server& svr)
{
    // 获取朋友圈首页（兼容 ShowDoc 原版 FristPage 拼写与 FirstPage）
    svr.Post("/TimelineGetFristPage", [](const httplib::Request& req, httplib::Response& res) {
        HandleGetTimeline(req, res, false);
    });

    svr.Post("/TimelineGetFirstPage", [](const httplib::Request& req, httplib::Response& res) {
        HandleGetTimeline(req, res, false);
    });

    // 获取朋友圈下一页
    svr.Post("/TimelineGetNextPage", [](const httplib::Request& req, httplib::Response& res) {
        HandleGetTimeline(req, res, true);
    });

    // 指定好友相册直接读取网络响应，不依赖本地缓存落盘。
    friend_timeline_route::Register(svr);
    sns_like_route::Register(svr);
}
