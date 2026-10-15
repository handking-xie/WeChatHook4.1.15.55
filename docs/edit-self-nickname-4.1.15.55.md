# EditSelfNickname — Weixin 4.1.15.55

日期：2026-10-15。当前项目原生实现及本轮验证；旧版结果不作为本轮依据。

## 使用及结果

POST /EditSelfNickname：{"nickname":"新昵称"}。兼容 nick_name，两者都存在时以 nickname 为准。
拒绝错误 JSON、非对象、缺少/非字符串参数、空字符串、NUL、无效 UTF-8 及超过 4096 字节。
4096 为本地防御上限，实际昵称业务限制由服务端决定。
生产原生 Edit 自身同样校验输入，绕过 HTTP 调用也不会提交非法昵称。

返回 ret、retmsg、submitted、verified。只有 Oplog 业务确认且后续 GetProfile 昵称匹配，
并且最终当前账号检查通过，才返回 ret=0、verified=true。
若服务端当前昵称本来已匹配，返回 ret=0、submitted=false、verified=true，不提交 Oplog。
submitted 仅表示昵称 Oplog 进入原生提交阶段，不包含只读 GetProfile 请求。

每个请求最多等待 30 秒，修改通常涉及读取、Oplog、回读共三次原生请求。
超时不自动重试，ret=-1，已提交时保留 submitted=true，明确结果未知；不把提交成功当业务完成。
verified 是服务端回读和账号检查，不等于 PC 界面缓存已刷新。

## 原生实现方式

include/EditSelfNickname.h → include/SelfQrCgi.h 的已核验新版 ABI → 当前原生 CGI 服务。
SelfQrCgi 最初用于本人二维码，本轮分别核验 GetProfile/Oplog 的新入口和对象布局后复用。
旧版 include/NativeCgi.h 的偏移未改，也没有扩大其它旧版操作接口的兼容范围。
include/EditSelfNicknameRoute.h 支持注入 editor 做生产 HTTP 测试，默认仍调用真正的 Edit。
src/MsgExtended.cpp 保持 RegisterEditSelfNickname 注册。

1. 使用 SelfProfileMemory 的新版构建/账号检查，获取当前 wxid。
2. 原生 GetProfile 查询完整 ModUserInfo，核对服务端 wxid 与当前账号相同。
3. 再核对当前账号；若昵称已相同直接返回已验证，避免重复写操作。
4. 通过 NicknameWire 只替换 ModUserInfo 的昵称字段 3，保留其余字段及未知字段原字节。
5. 按 Oplog CommandId=1 编码更新，原生提交前再次核对当前账号。
6. 校验 Oplog 外层 Ret=0、单个命令业务结果为 0；重新 GetProfile，确认昵称匹配。
7. 最后检查当前账号未改变，再发布成功。

不写客户端 account+0x88 或 SelfInfo 缓存，不用参数回显代替结果。
串行锁只约束本 DLL 的修改请求，不锁住客户端或其它 DLL。每次原生提交和回读均核对账号，
但前后快照不排除 ABA 切换或完整原生对象生命周期竞争；真实账号切换未运行验证。

## 4.1.15.55 专属入口及证据

实际目标 C:\Program Files\Tencent\Weixin\4.1.15.55\Weixin.dll，AMD64，
PE timestamp=0x6ACCBF76，SizeOfImage=0xC34F000。
SHA256=6fbbba536b04c0a24730710f540887a53f4eaa24398e4f94905d9b4a36148595。
构建校验不以目录名或 README 的旧版本描述为依据；当前 Edit 仅适配此新版构建。

本轮使用项目 pefile/Capstone 工具、当前微信原生 codec/网络和短时生产 HTTP 验证。
下表静态指令为普通本机工具观察，不称为 REA Evidence。证据保存在本项目 .review/nickname-*。

| 项目 | RVA / 布局 |
|---|---|
| GetProfile URI | 0x8FDC790，/cgi-bin/micromsg-bin/getprofile |
| GetProfile 异步提交 | 0x49A1C0；0x49A200 写 CGI 0x12E=302 |
| GetProfile scene 构造 / 虚表 | 0x49A500 / 0x8FDC488；分配 0xF0 字节 |
| GetProfile 请求构造 / 大小 | 0x1009600 / 32 字节 |
| GetProfile 响应构造 | 0x1009630；scene 中请求+0xA8，响应+0xC8 |
| GetProfile 请求复制 | 0x1020A60 |
| Oplog URI | 0x92E4720，/cgi-bin/micromsg-bin/oplog |
| Oplog 异步提交 | 0x19B7D40；0x19B7D80 写 CGI 0x2A9=681 |
| Oplog scene 构造 / 虚表 | 0x19B8080 / 0x92E4418；分配 0xE0 字节 |
| Oplog 请求构造 / 大小 | 0x3B1B040 / 24 字节 |
| Oplog 响应构造 | 0x3B1B060；scene 中请求+0xA8，响应+0xC0 |
| Oplog 请求复制 | 0x3B1C170 |
| CGI manager / service | *(base+0xBB92FC0)，service=*(manager+0x2C8)，虚表 0x8FCD388 |
| 原生部分解析 | 0x697160 |
| 原生分配 / sized free | 0x754C41C / 0x754C460；通过请求自身原生虚析构释放 |
| 响应序列化 | message 虚表 +0x40 计算长度、+0x50 写入自有缓冲区 |

提交 ABI：submit(service, request, nativeWxidString, options, functionStorage)。
两个独立提交函数均复制 request，接管 std::function 的 implementation（storage+0x38），
经 service 虚表+0x28 注册，再通过 0x691E60/0x692200 调度。未使用同步 coroutine 入口。
回调为 bool(implementation, scene**, errorType*, errorCode*)，scene 虚表+0x20 返回响应，
因此两个 scene 的响应对象偏移差异无需在本 DLL 中硬编码访问。

native string 32 字节，SSO 容量 15；options 48 字节，function storage 64 字节。
原生 request 在 submit 返回后析构，scene 已拥有其复制内容。
回调复制 PB 到本 DLL 的缓冲区，不持有原生响应指针。
共享状态与模块 pin 保留到晚到回调结束；等待超时不会取消原生任务，不会卸载回调代码。

## 本轮验证结果

- tools/nickname_wire_test.cpp：中文、未知字段/账号标志保留、截断、重复字段、账号不匹配、
  packed/unpacked 业务结果、命令数错误等协议检查通过。
- tools/nickname_route_test.cpp：相同生产 HTTP 注册函数测试通过，包括两个参数名及优先级、
  错误 JSON/类型、缺少字段、空/NUL/非法 UTF-8、超长昵称、非法请求不调用 backend、
  已提交但超时状态和同昵称不提交状态。
- tools/nickname_41555_native_test.cpp / nickname_41555_runner.py：新版 GetProfileRequest、
  OplogRequest 原生构造、部分解析、序列化、析构的往返测试通过。
- 原生异步 GetProfile 查询成功，返回的账号经匹配，昵称与客户端本地读取一致。
- 当前微信内短时随机本机端口的生产 HTTP 测试通过：两个参数名使用服务端当前昵称，
  ret=0、verified=true、submitted=false；非法请求均失败且不提交。
- 按本会话用户明确授权，真实改为 test，再恢复服务端原昵称。两次操作均 ret=0、
  submitted=true、verified=true；恢复后又单独 GetProfile 确认原昵称相同。
- 改名验证前保存 Windows DPAPI 加密恢复数据；确认恢复成功后删除，recovery_retained=false。
  日志仅保存状态、长度和匹配布尔值，没有明文原昵称、wxid、完整资料或会话密钥。
- MsgExtended.cpp 使用当前源码重新编译 Windows x64 对象通过；接口集合 JSON 解析及差异检查。

恢复实测记录摘要：edit_ret=0、edit_submitted=true、edit_verified=true；restore_ret=0、
restore_submitted=true、restore_verified=true；independent_restore_readback=true。
临时 HTTP 服务已停止；诊断 DLL 保留在当前进程，避免异步回调代码被卸载。

完整 Hook DLL 本轮未重新构建或替换部署；独立诊断 DLL 的生产路径实测不能当作代理 DLL
部署验收。其它构建、真实退出重登/账号切换、高并发、真实网络超时和晚到回调未实测。
不会主动刷新 PC 客户端界面，客户端内存资料可能短暂滞后于服务端。

## 复现

先把 Zig 的 global/local cache 设在当前项目 .review；新诊断 DLL 必须用新文件名或重新启动微信，
runner 会复用同名已加载模块，不应用旧模块验证新源码。

```powershell
.review/compiler/ziglang/zig.exe c++ -std=c++17 tools/nickname_wire_test.cpp -o .review/nickname_41555_wire_test.exe
.review/compiler/ziglang/zig.exe c++ -std=c++17 -Iinclude tools/nickname_route_test.cpp -o .review/nickname_41555_route_test.exe -lws2_32 -lcrypt32
.review/compiler/ziglang/zig.exe c++ -shared -std=c++17 -Iinclude tools/nickname_41555_native_test.cpp -o .review/nickname_41555_native.dll -lws2_32 -lcrypt32
# 每次重新枚举加载 Weixin.dll 的当前主进程，不能复用历史 PID。
python tools/nickname_41555_runner.py --pid <当前主进程PID> --codec
python tools/nickname_41555_runner.py --pid <当前主进程PID>
python tools/nickname_41555_runner.py --pid <当前主进程PID> --http
# 下面会真实改为 test 并恢复，仅在获得当前用户明确授权时运行。
python tools/nickname_41555_runner.py --pid <当前主进程PID> --edit-restore
```

此前 tools/nickname_native_test.cpp、nickname_native_runner.py 和旧状态文档属于 4.1.15.13
历史诊断；本轮新版复现只使用文件名带 41555 的工具，不用历史 DLL 证明当前源码。

后续签名适配将昵称与签名写入锁统一到 SelfProfileMutation.h；同一 DLL 的两类完整资料更新互斥，锁互通及昵称 HTTP 回归已验证。见 [签名记录](edit-self-signature-4.1.15.55.md)。
