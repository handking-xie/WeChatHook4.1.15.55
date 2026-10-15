#include "httplib.h"
#include "json.hpp"
#include <windows.h>
#include "wx_send.h"
#include "SendTextMsg.h"
#include "SendTextNoSrcRoute.h"

using json = nlohmann::json;

void Route_SendTextMsg(httplib::Server& svr)
{
    auto handleSendText = [](const httplib::Request& req, httplib::Response& res)
    {
        json reqJson;
        json resp;

        try
        {
            reqJson = json::parse(req.body);
        }
        catch (...)
        {
            resp["ret"] = -1;
            resp["msg"] = "invalid json";
            res.set_content(resp.dump(), "application/json");
            return;
        }

        // 兼容多种参数字段：toid（ShowDoc文档）、wxid、wxidorgid（原工程）
        std::string wxidorgid = reqJson.value("toid", "");
        if (wxidorgid.empty()) wxidorgid = reqJson.value("wxid", "");
        if (wxidorgid.empty()) wxidorgid = reqJson.value("wxidorgid", "");
        if (wxidorgid.empty()) wxidorgid = reqJson.value("gid", "");

        // 兼容 msg 和 content
        std::string msg = reqJson.value("msg", "");
        if (msg.empty()) msg = reqJson.value("content", "");

        if (wxidorgid.empty())
        {
            resp["ret"] = -1;
            resp["retmsg"] = "target toid/wxid/wxidorgid is required and cannot be empty";
            res.set_content(resp.dump(), "application/json");
            return;
        }

        int err = SafeSendTextRaw(wxidorgid.c_str(), msg.c_str());
        if (err == 0) {
            resp["ret"] = 0;
            resp["retmsg"] = "success";
        } else {
            resp["ret"] = -1;
            resp["retmsg"] = "native send failure (offset uncalibrated or exception caught)";
        }

        res.set_content(resp.dump(), "application/json");
    };

    svr.Post("/SendTextMsg", handleSendText);
    RegisterSendTextNoSrc(svr);

    // 发送群 @ 消息
    svr.Post("/SendAtMsg_NoSrc", [](const httplib::Request& req, httplib::Response& res)
    {
        json reqJson;
        json resp;

        try
        {
            reqJson = json::parse(req.body);
        }
        catch (...)
        {
            resp["ret"] = -1;
            resp["msg"] = "invalid json";
            res.set_content(resp.dump(), "application/json");
            return;
        }

        std::string gid = reqJson.value("gid", reqJson.value("chatroom", ""));
        std::string msg = reqJson.value("msg", reqJson.value("content", ""));

        if (gid.empty())
        {
            resp["ret"] = -1;
            resp["retmsg"] = "gid is required";
            res.set_content(resp.dump(), "application/json");
            return;
        }

        int err = SafeSendTextRaw(gid.c_str(), msg.c_str());
        if (err == 0) {
            resp["ret"] = 0;
            resp["retmsg"] = "success";
        } else {
            resp["ret"] = -1;
            resp["retmsg"] = "native send failure (offset uncalibrated or exception caught)";
        }
        res.set_content(resp.dump(), "application/json");
    });


    svr.Post("/Decode_Pic", [](const httplib::Request& req, httplib::Response& res)
        {
            json reqJson;
            json resp;

            try
            {
                reqJson = json::parse(req.body);
            }
            catch (...)
            {
                resp["ret"] = -1;
                resp["msg"] = "invalid json";
                res.set_content(resp.dump(), "application/json");
                return;
            }

            std::string src_path = reqJson.value("src_path", "");
            if (src_path.empty()) src_path = reqJson.value("src", "");

            std::string dst_path = reqJson.value("dst_path", "");
            if (dst_path.empty()) dst_path = reqJson.value("dst", "");

            if (src_path.empty() || dst_path.empty())
            {
                resp["ret"] = -1;
                resp["retmsg"] = "src_path and dst_path are required";
                res.set_content(resp.dump(), "application/json");
                return;
            }

            OutputDebugStringA(("Decode_Pic src_path: " + src_path + "\n").c_str());
            OutputDebugStringA(("Decode_Pic dst_path: " + dst_path + "\n").c_str());

            WeixinSend::DecodePic(src_path, dst_path);

            resp["ret"] = 0;
            resp["retmsg"] = "success";

            res.set_content(resp.dump(), "application/json");
        });
}
