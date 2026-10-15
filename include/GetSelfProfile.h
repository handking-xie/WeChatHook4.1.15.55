#pragma once
#include <string>
#include "global.h"

namespace httplib { class Server; }

// 4.1.15.13 native account offsets only; no database/path/global-cache fallback.
// Currently verified fields: wxid, alias, nickname, phone.
bool GetSelfProfile(SelfInfo_t& outInfo);

// 注册 /GetSelfProfile 接口
void RegisterGetSelfProfile(httplib::Server& svr);
