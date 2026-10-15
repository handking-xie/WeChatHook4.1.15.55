#include "httplib.h"
#include "json.hpp"
#include <windows.h>
#include <string>
#include <fstream>
#include "global.h"
#include "tools.h"
#include "ProfileMemory.h"

#include "GetSelfProfile.h"

using json = nlohmann::json;


static bool ReadSelfProfile(SelfInfo_t& outInfo, std::string& error)
{
    outInfo = {};
    profile_memory::Profile profile;
    const uintptr_t base = reinterpret_cast<uintptr_t>(GetModuleHandleW(L"Weixin.dll"));
    if (!profile_memory::Read(GetCurrentProcess(), base, profile, error)) return false;
    outInfo.wxid = std::move(profile.wxid);
    outInfo.alias = std::move(profile.alias);
    outInfo.nickname = std::move(profile.nickname);
    outInfo.phone = std::move(profile.phone);
    return true;
}

bool GetSelfProfile(SelfInfo_t& outInfo)
{
    std::string error;
    return ReadSelfProfile(outInfo, error);
}

/* =========================
   本人二维码辅助处理（支持 ShowDoc 标准响应、内存实时截获与持久化缓存）
   ========================= */
static int SafeProbeChunk(const char* buf, size_t size, char* outUrl, int maxOut)
{
    __try {
        const char target1[] = "https://u.wechat.com/M";
        const size_t len1 = sizeof(target1) - 1;
        const char target2[] = "http://weixin.qq.com/r/";
        const size_t len2 = sizeof(target2) - 1;
        const char target3[] = "https://weixin.qq.com/r/";
        const size_t len3 = sizeof(target3) - 1;

        for (size_t i = 0; i + 30 <= size; ++i)
        {
            if (buf[i] == 'h' && buf[i + 1] == 't' && buf[i + 2] == 't' && buf[i + 3] == 'p')
            {
                if (memcmp(buf + i, target1, len1) == 0)
                {
                    size_t j = i + len1;
                    while (j < size && (unsigned char)buf[j] > 32 && buf[j] != '"' && buf[j] != '\'' && buf[j] != '<' && buf[j] != '>')
                        ++j;
                    int len = (int)(j - i);
                    if (len >= (int)len1 + 5 && len < maxOut)
                    {
                        memcpy(outUrl, buf + i, len);
                        outUrl[len] = '\0';
                        return len;
                    }
                }
                else if (memcmp(buf + i, target2, len2) == 0 || memcmp(buf + i, target3, len3) == 0)
                {
                    size_t prefLen = (buf[i + 4] == 's') ? len3 : len2;
                    size_t j = i + prefLen;
                    while (j < size && (unsigned char)buf[j] > 32 && buf[j] != '"' && buf[j] != '\'' && buf[j] != '<' && buf[j] != '>')
                        ++j;
                    int len = (int)(j - i);
                    if (len >= (int)prefLen + 5 && len < maxOut)
                    {
                        memcpy(outUrl, buf + i, len);
                        outUrl[len] = '\0';
                        return len;
                    }
                }
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return 0;
    }
    return 0;
}

static std::string GetQRCodeCachePath()
{
    char modPath[MAX_PATH] = { 0 };
    if (GetModuleFileNameA(g_hModule, modPath, MAX_PATH))
    {
        std::string p = modPath;
        size_t pos = p.find_last_of("\\/");
        if (pos != std::string::npos)
        {
            return p.substr(0, pos + 1) + "qrcode_cache.json";
        }
    }
    return "qrcode_cache.json";
}

static std::string ReadCachedQRCode()
{
    std::string path = GetQRCodeCachePath();
    std::ifstream ifs(path);
    if (ifs.is_open())
    {
        try
        {
            json j;
            ifs >> j;
            if (j.contains("QRCode_URL") && j["QRCode_URL"].is_string())
            {
                std::string url = j["QRCode_URL"].get<std::string>();
                if (!url.empty()) return url;
            }
        }
        catch (...) {}
    }
    return "";
}

static void SaveCachedQRCode(const std::string& url)
{
    if (url.empty()) return;
    std::string path = GetQRCodeCachePath();
    try
    {
        json j;
        j["QRCode_URL"] = url;
        j["update_time"] = GetTickCount64();
        std::ofstream ofs(path);
        if (ofs.is_open())
        {
            ofs << j.dump(4);
        }
    }
    catch (...) {}
}

static std::string ScanMemoryForQRCode()
{
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    uintptr_t addr = (uintptr_t)si.lpMinimumApplicationAddress;
    uintptr_t maxAddr = (uintptr_t)si.lpMaximumApplicationAddress;

    char urlBuf[512] = { 0 };
    MEMORY_BASIC_INFORMATION mbi;
    while (addr < maxAddr && VirtualQuery((LPCVOID)addr, &mbi, sizeof(mbi)))
    {
        if (mbi.State == MEM_COMMIT &&
            (mbi.Protect == PAGE_READONLY || mbi.Protect == PAGE_READWRITE || mbi.Protect == PAGE_EXECUTE_READ || mbi.Protect == PAGE_EXECUTE_READWRITE) &&
            !(mbi.Protect & PAGE_GUARD))
        {
            int n = SafeProbeChunk((const char*)mbi.BaseAddress, mbi.RegionSize, urlBuf, sizeof(urlBuf));
            if (n > 0)
            {
                return std::string(urlBuf, n);
            }
        }
        addr = (uintptr_t)mbi.BaseAddress + mbi.RegionSize;
    }
    return "";
}

/* =========================
   HTTP 接口
   ========================= */

void RegisterGetSelfProfile(httplib::Server& svr)
{
    svr.Post("/GetSelfProfile", [](const httplib::Request&, httplib::Response& res)
        {
            SelfInfo_t profile{};
            std::string error;
            if (!ReadSelfProfile(profile, error)) {
                json failure = {{"ret", -1}, {"retmsg", error}};
                res.set_content(failure.dump(), "application/json");
                return;
            }

            json resp;
            resp["ret"] = 0;
            resp["retmsg"] = "success";
            resp["unavailable_fields"] = {"email", "qq", "proiv", "area", "signinfo"};
            resp["wxid"] = profile.wxid;
            resp["alias"] = profile.alias;
            resp["nickname"] = profile.nickname;
            resp["email"] = profile.email;
            resp["qq"] = profile.qq;
            resp["phone"] = profile.phone;
            resp["proiv"] = profile.proiv;
            resp["area"] = profile.area;
            resp["signinfo"] = profile.signinfo;
            
            res.set_content(resp.dump(), "application/json");
        });

    svr.Post("/GetSelfAddress", [](const httplib::Request&, httplib::Response& res)
        {
            SelfInfo_t profile;
            GetSelfProfile(profile);

            json resp;
            resp["province"] = profile.proiv;
            resp["city"] = profile.area;
            resp["county"] = "";
            resp["postcode"] = "";
            resp["address"] = "";
            resp["contact"] = profile.nickname;
            resp["phone"] = profile.phone;

            res.set_content(resp.dump(), "application/json");
        });

    svr.Post("/getonlineinfo", [](const httplib::Request&, httplib::Response& res)
        {
            SelfInfo_t profile;
            bool loggedIn = GetSelfProfile(profile);

            json resp;
            resp["ret"] = 0;
            resp["online_status"] = loggedIn ? 1 : 0;
            resp["is_online"] = loggedIn;
            resp["device"] = "PC";

            res.set_content(resp.dump(), "application/json");
        });



    svr.Post("/GetQRCode", [](const httplib::Request& req, httplib::Response& res)
        {
            SelfInfo_t profile;
            GetSelfProfile(profile);

            // 1. 如果请求体传入了 QRCode_URL 或 qr_url，直接保存并使用
            std::string customUrl;
            if (!req.body.empty())
            {
                try
                {
                    json bodyJson = json::parse(req.body);
                    if (bodyJson.contains("QRCode_URL") && bodyJson["QRCode_URL"].is_string())
                        customUrl = bodyJson["QRCode_URL"].get<std::string>();
                    else if (bodyJson.contains("qr_url") && bodyJson["qr_url"].is_string())
                        customUrl = bodyJson["qr_url"].get<std::string>();
                }
                catch (...) {}
            }

            if (!customUrl.empty())
            {
                SaveCachedQRCode(customUrl);
            }

            // 2. 检查持久化缓存
            std::string qrUrl = ReadCachedQRCode();

            // 3. 内存动态实时扫描探测（若用户在微信客户端曾打开过“我的二维码”）
            if (qrUrl.empty())
            {
                qrUrl = ScanMemoryForQRCode();
                if (!qrUrl.empty())
                {
                    SaveCachedQRCode(qrUrl);
                }
            }

            // 4. 构建标准 ShowDoc 响应
            json resp;
            resp["ret"] = 0;
            resp["code"] = 0;
            resp["wxid"] = profile.wxid;
            resp["QRCode_URL"] = qrUrl;
            resp["qr_url"] = qrUrl;

            if (qrUrl.empty())
            {
                resp["msg"] = "QR code link not detected in memory yet. Please open 'My QR Code' once in WeChat PC to auto-capture, or POST { \"QRCode_URL\": \"...\" } to set it.";
            }

            res.set_content(resp.dump(), "application/json");
        });

    svr.Post("/SetQRCode", [](const httplib::Request& req, httplib::Response& res)
        {
            json resp;
            try
            {
                json body = json::parse(req.body);
                std::string url = body.value("QRCode_URL", body.value("qr_url", ""));
                if (!url.empty())
                {
                    SaveCachedQRCode(url);
                    resp["ret"] = 0;
                    resp["code"] = 0;
                    resp["msg"] = "success";
                    resp["QRCode_URL"] = url;
                }
                else
                {
                    resp["ret"] = -1;
                    resp["msg"] = "QRCode_URL is required";
                }
            }
            catch (...)
            {
                resp["ret"] = -1;
                resp["msg"] = "invalid json";
            }
            res.set_content(resp.dump(), "application/json");
        });
}

