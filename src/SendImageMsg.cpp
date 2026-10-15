#include "httplib.h"
#include "json.hpp"
#include <windows.h>
#include "wx_send.h"
#include "SendImageMsg.h"

using json = nlohmann::json;

void Route_SendImageMsg(httplib::Server& svr)
{
    auto handleSendImg = [](const httplib::Request& req, httplib::Response& res)
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

        // 兼容多种接收人字段名
        std::string wxidorgid = reqJson.value("toid", "");
        if (wxidorgid.empty()) wxidorgid = reqJson.value("wxid", "");
        if (wxidorgid.empty()) wxidorgid = reqJson.value("wxidorgid", "");

        // 兼容多种图片路径字段名
        std::string path = reqJson.value("path", "");
        if (path.empty()) path = reqJson.value("image_path", "");
        if (path.empty()) path = reqJson.value("img_path", "");
        if (path.empty()) path = reqJson.value("filepath", "");

        if (wxidorgid.empty() || path.empty())
        {
            resp["ret"] = -1;
            resp["retmsg"] = "target toid/wxid and image path are required";
            res.set_content(resp.dump(), "application/json");
            return;
        }

        if (GetFileAttributesA(path.c_str()) == INVALID_FILE_ATTRIBUTES)
        {
            resp["ret"] = -1;
            resp["retmsg"] = "image file not found: " + path;
            res.set_content(resp.dump(), "application/json");
            return;
        }
        
        int err = SafeSendImageRaw(wxidorgid.c_str(), path.c_str());
        if (err == 0) {
            resp["ret"] = 0;
            resp["retmsg"] = "success";
        } else {
            resp["ret"] = -1;
            resp["retmsg"] = "native send failure (offset uncalibrated or exception caught)";
        }

        res.set_content(resp.dump(), "application/json");
    };

    svr.Post("/SendImgMsg", handleSendImg);
    svr.Post("/SendImgMsg_NoSrc", handleSendImg);
}

