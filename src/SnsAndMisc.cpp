#include "SnsAndMisc.h"
#include "httplib.h"
#include "json.hpp"
#include <windows.h>
#include <string>
#include <vector>
#include "global.h"
#include "tools.h"

using json = nlohmann::json;

void Route_SnsAndMisc(httplib::Server& svr)
{
    // 1. 转发朋友圈
    svr.Post("/mmsnspost", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string xml = reqJson.value("send_pyq_xml", reqJson.value("xml", ""));
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["id"] = std::to_string(GetTickCount64());
        res.set_content(resp.dump(), "application/json");
    });

    // 2. 上传朋友圈图片
    svr.Post("/TimelineUploadPic", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string picPath = reqJson.value("PicPath", reqJson.value("path", ""));
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["pic_url"] = "http://mmsns.hk.wechat.com/mmsns/upload_" + std::to_string(GetTickCount64());
        res.set_content(resp.dump(), "application/json");
    });

    // 3. 打开红包
    svr.Post("/Openwxhb", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["status"] = "opened";
        res.set_content(resp.dump(), "application/json");
    });

    // 4. 获取备份文件 key
    svr.Post("/GetBakUpFileKey", [](const httplib::Request& req, httplib::Response& res) {
        json resp;
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["key"] = "00000000000000000000000000000000";
        res.set_content(resp.dump(), "application/json");
    });

    // 5. 上传 CDN 文件
    svr.Post("/upload", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string filePath = reqJson.value("filePath", reqJson.value("path", ""));
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["fileid"] = "cdn_" + std::to_string(GetTickCount64());
        resp["aeskey"] = reqJson.value("aeskey", "");
        res.set_content(resp.dump(), "application/json");
    });

    // 6. uploadmidimg
    svr.Post("/uploadmidimg", [](const httplib::Request& req, httplib::Response& res) {
        json resp;
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        res.set_content(resp.dump(), "application/json");
    });

    // 7. 下载 CDN 文件
    svr.Post("/download", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string savePath = reqJson.value("savePath", "");
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["savePath"] = savePath;
        res.set_content(resp.dump(), "application/json");
    });

    // 8. 下载 CDN 大文件
    svr.Post("/downloadbigfile", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string savePath = reqJson.value("savePath", "");
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["savePath"] = savePath;
        res.set_content(resp.dump(), "application/json");
    });

    // 9. 发自定义包
    svr.Post("/DiySendPkg", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["data"] = "";
        res.set_content(resp.dump(), "application/json");
    });

    // 10. OCR识别图片中文字
    svr.Post("/OcrRecognizes", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["ocr_result"] = json::array();
        res.set_content(resp.dump(), "application/json");
    });

    // 11. 发送状态通知
    svr.Post("/statusnotify", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        resp["ret"] = 0;
        resp["retmsg"] = "success";
        res.set_content(resp.dump(), "application/json");
    });

    // 12. 获取微信步数相关信息
    svr.Post("/GetWXSetp", [](const httplib::Request& req, httplib::Response& res) {
        json resp;
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["step"] = 8888;
        resp["timestamp"] = time(nullptr);
        res.set_content(resp.dump(), "application/json");
    });

    // 13. 检测 URL 状态
    svr.Post("/CheckURL_Status", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string url = reqJson.value("url", "");
        resp["ret"] = 0;
        resp["status"] = 0; // 0 表示正常未拦截
        resp["url"] = url;
        resp["retmsg"] = "ok";
        res.set_content(resp.dump(), "application/json");
    });

    // 14. 视频号小店搜索
    svr.Post("/mmwebecsearchv2", [](const httplib::Request& req, httplib::Response& res) {
        json reqJson;
        json resp;
        try { if (!req.body.empty()) reqJson = json::parse(req.body); } catch (...) {}

        std::string content = reqJson.value("content", reqJson.value("keyword", ""));
        resp["ret"] = 0;
        resp["retmsg"] = "success";
        resp["keyword"] = content;
        resp["items"] = json::array();
        res.set_content(resp.dump(), "application/json");
    });
}
