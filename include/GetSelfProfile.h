#pragma once
#include <string>
#include "global.h"

namespace httplib { class Server; }

// 4.1.15.55 native account offsets only; no database/path/global-cache fallback.
// Reads wxid, alias, nickname and phone candidate; see docs/self-profile-4.1.15.55.md.
bool GetSelfProfile(SelfInfo_t& outInfo);

// 注册 /GetSelfProfile 接口
void RegisterGetSelfProfile(httplib::Server& svr);
