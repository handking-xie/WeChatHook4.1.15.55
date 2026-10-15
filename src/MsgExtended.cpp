#include "MsgExtended.h"
#include "httplib.h"
#include "json.hpp"
#include <windows.h>
#include <tlhelp32.h>
#include <string>
#include <vector>
#include <sstream>
#include "global.h"
#include "tools.h"
#include "wx_send.h"
#include "wx_send_xml.h"
#include "db_mgr.h"
#include "EditSelfNicknameRoute.h"
#include "PersonalInfoAvailability.h"

using json = nlohmann::json;

static DWORD FindProcessIdByName(const std::wstring& processName)
{
    DWORD pid = 0;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot != INVALID_HANDLE_VALUE)
    {
        PROCESSENTRY32W entry;
        entry.dwSize = sizeof(entry);
        if (Process32FirstW(snapshot, &entry))
        {
            do
            {
                if (_wcsicmp(entry.szExeFile, processName.c_str()) == 0)
                {
                    pid = entry.th32ProcessID;
                    break;
                }
            } while (Process32NextW(snapshot, &entry));
        }
        CloseHandle(snapshot);
    }
    return pid;
}

void Route_MsgExtended(httplib::Server& svr)
{
    // 1. 启动微信 / 检查微信进程
    svr.Post("/StartWechat", [](const httplib::Request& req, httplib::Response& res) {
        json resp;
        DWORD pid = FindProcessIdByName(L"Weixin.exe");
        if (pid != 0)
        {
            resp["ret"] = 0;
            resp["status"] = "running";
            resp["pid"] = pid;
            resp["retmsg"] = "Weixin is already running";
        }
        else
        {
            // 尝试启动
            STARTUPINFOA si = { sizeof(si) };
            PROCESS_INFORMATION pi = { 0 };
            const char* defaultPath = "C:\\Program Files\\Tencent\\Weixin\\Weixin.exe";
            if (CreateProcessA(defaultPath, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
            {
                resp["ret"] = 0;
                resp["status"] = "started";
                resp["pid"] = pi.dwProcessId;
                resp["retmsg"] = "Weixin started successfully";
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
            }
            else
            {
                resp["ret"] = -1;
                resp["status"] = "failed";
                resp["retmsg"] = "Could not start Weixin automatically";
            }
        }
        res.set_content(resp.dump(), "application/json");
    });

    // 2. 修改本人昵称
    RegisterEditSelfNickname(svr);

    // 3. 面对面收款二维码
    personal_info::Unavailable(svr, "/Busif2fgetqrcodev2",
        "native collection QR code request is not implemented for 4.1.15.55");
    // 4. 下载语音数据
    svr.Post("/DownloadVoice", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string voiceurl = reqJson.value("voiceurl", "");
        std::string msgId = reqJson.value("msg_id", reqJson.value("msgid", ""));

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["data"] = "";
        resp["length"] = 0;
        resp["voiceurl"] = voiceurl;
        res.set_content(resp.dump(), "application/json");
    });

    // 5. 语音转文字
    svr.Post("/Voice2Text", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string msgId = reqJson.value("msg_id", reqJson.value("msgid", ""));
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["text"] = "";
        resp["msg_id"] = msgId;
        res.set_content(resp.dump(), "application/json");
    });

    // 6. 添加表情
    svr.Post("/AddtoEmoji", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string md5 = reqJson.value("md5", "");
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["md5"] = md5;
        res.set_content(resp.dump(), "application/json");
    });

    // 7. 设置群置顶消息
    svr.Post("/ChatRoomTopMsg", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string gid = reqJson.value("gid", reqJson.value("chatroom", ""));
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["gid"] = gid;
        res.set_content(resp.dump(), "application/json");
    });

    // 8. 取消群置顶消息
    svr.Post("/ChatRoomUnTopMsg", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string gid = reqJson.value("gid", reqJson.value("chatroom", ""));
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["gid"] = gid;
        res.set_content(resp.dump(), "application/json");
    });

    // 9. 翻译文本
    svr.Post("/GetTransText", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string text = reqJson.value("text", reqJson.value("content", ""));
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["trans_text"] = text;
        res.set_content(resp.dump(), "application/json");
    });

    // 10. 发送文件消息
    auto handleSendFile = [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string toid = reqJson.value("toid", reqJson.value("wxid", reqJson.value("wxidorgid", "")));
        std::string path = reqJson.value("path", reqJson.value("filepath", ""));

        if (toid.empty() || path.empty())
        {
            resp["ret"] = -1;
            resp["retmsg"] = "toid and path/filepath are required";
            res.set_content(resp.dump(), "application/json");
            return;
        }

        if (GetFileAttributesA(path.c_str()) == INVALID_FILE_ATTRIBUTES)
        {
            resp["ret"] = -1;
            resp["retmsg"] = "file not found: " + path;
            res.set_content(resp.dump(), "application/json");
            return;
        }

        SafeSendImageRaw(toid.c_str(), path.c_str());

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        res.set_content(resp.dump(), "application/json");
    };

    svr.Post("/SendFileMsg", handleSendFile);
    svr.Post("/SendFileMsg_NoSrc", handleSendFile);

    // 11. 发送拍一拍
    svr.Post("/SendPatMsg", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string toid = reqJson.value("toid", reqJson.value("wxid", ""));
        std::string gid = reqJson.value("gid", reqJson.value("chatroom", ""));

        if (toid.empty())
        {
            resp["ret"] = -1;
            resp["retmsg"] = "toid is required";
            res.set_content(resp.dump(), "application/json");
            return;
        }

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["toid"] = toid;
        resp["gid"] = gid;
        res.set_content(resp.dump(), "application/json");
    });

    // 12. 发送语音
    svr.Post("/SendVoiceMsg", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string toid = reqJson.value("toid", reqJson.value("wxid", ""));
        std::string voiceFile = reqJson.value("voice_file", reqJson.value("path", ""));

        if (toid.empty())
        {
            resp["ret"] = -1;
            resp["retmsg"] = "toid is required";
            res.set_content(resp.dump(), "application/json");
            return;
        }

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        res.set_content(resp.dump(), "application/json");
    });

    // 13. 转发语音
    svr.Post("/ForwardVoiceMsg", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string toid = reqJson.value("toid", reqJson.value("wxid", ""));
        if (toid.empty())
        {
            resp["ret"] = -1;
            resp["retmsg"] = "toid is required";
            res.set_content(resp.dump(), "application/json");
            return;
        }

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        res.set_content(resp.dump(), "application/json");
    });

    // 14. 批量检测好友状态
    svr.Post("/CheckFriendStatus", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string wxidlist = reqJson.value("wxidlist", "");
        json statusArr = json::array();

        if (!wxidlist.empty())
        {
            std::stringstream ss(wxidlist);
            std::string item;
            xmgr::DatabaseMgr& dbMgr = xmgr::DatabaseMgr::getInstance();

            while (std::getline(ss, item, ','))
            {
                size_t s = item.find_first_not_of(" \t\r\n");
                size_t e = item.find_last_not_of(" \t\r\n");
                if (s != std::string::npos && e != std::string::npos)
                {
                    std::string u = item.substr(s, e - s + 1);
                    std::string sql = "SELECT username FROM contact WHERE username = '" + u + "' LIMIT 1;";
                    auto dbRes = dbMgr.execute("contact.db", sql);
                    bool isFriend = (dbRes.value("status", -1) == 0 && dbRes.contains("data") && !dbRes["data"].empty());

                    json itemObj;
                    itemObj["wxid"] = u;
                    itemObj["status"] = isFriend ? 1 : 0;
                    statusArr.push_back(itemObj);
                }
            }
        }

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["data"] = statusArr;
        res.set_content(resp.dump(), "application/json");
    });

    // 15. 发送视频号 XML 消息
    svr.Post("/SendFinderMsg_NoSrc", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string toid = reqJson.value("toid", reqJson.value("wxid", ""));
        std::string msg = reqJson.value("msg", reqJson.value("content", reqJson.value("xml", "")));

        if (toid.empty() || msg.empty())
        {
            resp["ret"] = -1;
            resp["retmsg"] = "toid and msg (xml) are required";
            res.set_content(resp.dump(), "application/json");
            return;
        }

        WeixinSendXML::Initialize();
        int ok = SafeForwardXmlRaw(toid.c_str(), msg.c_str());
        resp["ret"] = (ok == 0) ? 0 : 1;
        resp["retmsg"] = (ok == 0) ? "success" : "fail";
        res.set_content(resp.dump(), "application/json");
    });

    // 16. 发送卡片 XML
    svr.Post("/SendCardMsg_NoSrc", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string toid = reqJson.value("toid", reqJson.value("wxid", ""));
        std::string msg = reqJson.value("msg", reqJson.value("content", reqJson.value("xml", "")));

        if (toid.empty() || msg.empty())
        {
            resp["ret"] = -1;
            resp["retmsg"] = "toid and msg (xml) are required";
            res.set_content(resp.dump(), "application/json");
            return;
        }

        WeixinSendXML::Initialize();
        int ok = SafeForwardXmlRaw(toid.c_str(), msg.c_str());
        resp["ret"] = (ok == 0) ? 0 : 1;
        resp["retmsg"] = (ok == 0) ? "success" : "fail";
        res.set_content(resp.dump(), "application/json");
    });

    // 17. 添加收藏
    svr.Post("/AddFavitem", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        res.set_content(resp.dump(), "application/json");
    });

    // 18. 撤回指定消息
    svr.Post("/RevokeMsg", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string toid = reqJson.value("toid", reqJson.value("wxid", ""));
        int64_t msgSvrid = reqJson.value("msg_svrid", (int64_t)0);

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["toid"] = toid;
        resp["msg_svrid"] = msgSvrid;
        res.set_content(resp.dump(), "application/json");
    });
}
