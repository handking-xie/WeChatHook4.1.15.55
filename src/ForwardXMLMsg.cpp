#include "json.hpp"
#include "httplib.h"

#include <windows.h>
#include "wx_send_xml.h"

#include "ForwardXMLMsg.h"

using json = nlohmann::json;

void Route_ForwardXMLMsg(httplib::Server& svr)
{
    svr.Post("/ForwardXMLMsg", [](const httplib::Request& req, httplib::Response& res)
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

            std::string wxid = reqJson.value("to_wxid", "");
            if (wxid.empty()) wxid = reqJson.value("toid", "");
            if (wxid.empty()) wxid = reqJson.value("wxid", "");
            if (wxid.empty()) wxid = reqJson.value("wxidorgid", "");

            std::string xml = reqJson.value("content", "");
            if (xml.empty()) xml = reqJson.value("xml", "");
            if (xml.empty()) xml = reqJson.value("msg", "");

            if (wxid.empty() || xml.empty())
            {
                resp["ret"] = -1;
                resp["retmsg"] = "to_wxid and content (xml) are required";
                res.set_content(resp.dump(), "application/json");
                return;
            }

            // 后续调用
            WeixinSendXML::Initialize();
            int err = SafeForwardXmlRaw(wxid.c_str(), xml.c_str());

            if (err == 0) {
                resp["ret"] = 0;
                resp["retmsg"] = "success";
            }
            else {
                resp["ret"] = 1;
                resp["retmsg"] = "fail";
            }

            res.set_content(resp.dump(), "application/json");
        });
}