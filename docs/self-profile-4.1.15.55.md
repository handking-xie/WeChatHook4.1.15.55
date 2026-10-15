# GetSelfProfile — Weixin 4.1.15.55

日期：2026-10-15。接口集合：本项目 PCWeixinHook_ShowDoc_All_Apifox_Merged.apifox.json 中“个人信息 / 获取个人信息”。

## 实现

POST /GetSelfProfile，无参数，请求体可为 {}。
include/SelfProfileMemory.h 负责原生账号读取，include/SelfProfileRoute.h 负责响应及路由注册，src/GetSelfProfile.cpp 调用注册函数，并将读取结果提供给 GetSelfProfile。
旧版 include/ProfileMemory.h 的 Read 仍限定 4.1.15.13，避免启用其它尚未适配的原生操作；新版仅复用其安全内存和字符串工具。

当前构建校验：AMD64、PE timestamp=0x6ACCBF76、SizeOfImage=0xC34F000。
manager=*(Weixin.dll+0xBB92FC0)，account=*(manager+0x68)，账号虚表必须为模块基址+0x8FD1498，登录位为 account+0x38 的 bit 0。
指针链及 wxid/alias getter 依据见本项目 docs/is-login-status-4.1.15.55.md。
本次使用已有 .review/login_analysis.py 只读检查已安装模块：虚表 +0x18/+0x20 分别指向 RVA 0x3855C0/0x3855D0，返回 account+0x48/+0x68。REA 工具未连接；这里为本机 pefile/Capstone 观察，不是 REA Evidence。

字段：wxid(+0x48)、alias(+0x68)、nickname(+0x88)、phone(+0xA8)，均按 MSVC 32 字节字符串读取，校验长度、容量、UTF-8，并复读字段。
nickname 复用已核对账号对象中的相邻字符串位置；phone 在本次同一对象中读取到 11 字节字符串候选，与旧版布局一致，但尚未与账号设置独立核对，手机号语义仍有此验证限制。
生产路径不扫描内存、不查询数据库、不读取目录或全局 SelfInfo 缓存。
读取完成前重新校验 manager、account、虚表、登录位和全部四个字段；失败清空输出。
一致性检查不持有微信内部锁，无法排除 ABA 变化及完整对象生命周期竞争。

## 返回

成功：ret=0、retmsg=success，返回 wxid、alias、nickname、phone。
兼容字段 email/proiv/area/signinfo 返回空字符串，qq 返回 0；这些占位值不代表真实账号值，unavailable_fields 明确列出 email、qq、proiv、area、signinfo。
未登录、错误构建、不可读/错误布局、非法字符串或读取期间变化：ret=-1、具体 retmsg，不返回任何账号字段。
仅实现 HTTP POST；未新增 WebSocket 分发。

## 验证

工具 tools/self_profile_test.cpp 复用生产读取函数和注册函数。模拟覆盖内联/堆字符串、中文、退出、错误构建/虚表、非法 UTF-8、超长/不可读字符串、空 wxid/账号及失败清空；本机临时 HTTP 服务覆盖成功、退出和错误构建。
实时模式重新枚举 PID/模块基址后，以 PROCESS_VM_READ | PROCESS_QUERY_INFORMATION 读取微信；只输出长度，不保存个人值，不注入 DLL、不修改内存、不发送业务消息。

首次生产读取模拟测试通过；当前已运行微信原生读取成功，wxid/alias/nickname/phone 字节长度分别为 19/14/6/11。
完整 Hook DLL 尚未构建、替换部署；退出重登、账号切换及高并发竞争未实测。

最终测试：Windows x64 Zig 0.16.0 测试程序编译通过；模拟读取及真实本机 HTTP 成功/退出/错误构建测试通过；重新枚举微信进程后生产 Read 和相同 Register 的实时 HTTP 检查通过。仅输出字段长度。

GetSelfProfile.cpp 使用 -fms-extensions（既有二维码扫描使用 SEH）对象编译通过，仅有既有第三方字面量运算符警告。Apifox 集合 JSON 解析及 git diff --check 通过。
