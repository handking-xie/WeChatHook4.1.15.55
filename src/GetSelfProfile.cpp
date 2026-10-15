#include "httplib.h"
#include "json.hpp"
#include <windows.h>
#include <string>
#include "global.h"
#include "tools.h"
#include "SelfProfileRoute.h"
#include "OnlineInfoRoute.h"
#include "PersonalInfoAvailability.h"

#include "GetSelfProfile.h"

using json = nlohmann::json;


static bool ReadSelfProfile(SelfInfo_t& outInfo, std::string& error)
{
    outInfo = {};
    self_profile_memory::Profile profile;
    const uintptr_t base = reinterpret_cast<uintptr_t>(GetModuleHandleW(L"Weixin.dll"));
    if (!self_profile_memory::Read(GetCurrentProcess(), base, profile, error)) return false;
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

void RegisterGetSelfProfile(httplib::Server& svr)
{
    self_profile_memory::Register(svr);
    online_info::Register(svr);
    personal_info::Unavailable(svr, "/GetSelfAddress",
        "native delivery address request is not implemented for 4.1.15.55");
    personal_info::Unavailable(svr, "/GetQRCode",
        "native self QR code request is not implemented for 4.1.15.55");
    personal_info::Unavailable(svr, "/SetQRCode",
        "manual QR code injection is disabled; it cannot verify the current account QR code");
    personal_info::Unavailable(svr, "/api/mod_self_nick_signature",
        "native signature update is not implemented for 4.1.15.55");
    personal_info::Unavailable(svr, "/api/upload_head_img",
        "native avatar upload is not implemented for 4.1.15.55");
    personal_info::Unavailable(svr, "/api/get_config_path",
        "native configuration directory getter is not verified for 4.1.15.55");
}