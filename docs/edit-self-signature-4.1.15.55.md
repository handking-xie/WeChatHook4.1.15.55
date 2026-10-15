# 修改个人签名 — Weixin 4.1.15.55

日期：2026-10-15。接口 POST /api/mod_self_nick_signature，参数 newSignature。

## 合约

请求 {"newSignature":"新签名"}；空字符串表示清空签名。
要求 JSON 对象且 newSignature 为字符串；拒绝缺少字段、其它类型、NUL、非法 UTF-8、
超过 4096 字节的内容。4096 是本地防御上限，服务端可以施加更严格的业务限制。

返回 ret、retmsg、submitted、verified，并保留 Apifox 示例的 errCode/errMsg。
成功 ret=0、verified=true、errCode=1；失败 ret=-1、verified=false、errCode=-1。
errCode 是旧 HTTP 合约的成功标志，与微信业务 Ret=0 的编号不同。
若服务端签名已相同，submitted=false，直接返回已核验，不重复提交 Oplog。
submitted 表示签名更新 Oplog 已进入原生提交阶段，不包括只读 GetProfile。

传输完成不等于业务完成；只有 Oplog 确认单个命令成功、GetProfile 签名回读匹配、
昵称保持一致和本机账号检查通过才发布成功。不在响应中回显输入签名。
每次 CGI 最多等待 30 秒，完整更新通常包含三个原生请求；超时结果未知，不自动重试。
超时可能已经修改服务端，submitted=true 不能解释为操作被取消。

## 实现方式

src/GetSelfProfile.cpp → EditSelfSignatureRoute.h → EditSelfSignature.h →
已核验的 self_nickname::Native / SelfQrCgi.h 新版 GetProfile/Oplog。
SignatureWire.h 负责字段读取和更新编码；SelfProfileMutation.h 提供本 DLL 资料写入共享锁。
EditSelfNickname.h 同步改用该锁，避免本 DLL 的昵称和签名完整资料更新互相覆盖。
锁覆盖初读、更新、回读整个流程；并发写操作立即失败，不等待未知完成状态。
不锁住其它 DLL、其它客户端或超时后仍在原生队列中的操作。

```text
校验 newSignature（允许空字符串）
 → 获取昵称/签名共享写入锁，核验 4.1.15.55 构建和本机账号
 → GetProfile 查询完整 ModUserInfo，核对服务端 wxid
 → 读取 field 13 签名，复核当前账号；相同时不提交
 → 保留其它字段及未知字段原字节，仅替换或新增 field 13
 → Oplog CmdId=1，检查原生完成状态及单个命令业务结果
 → 再 GetProfile，核对签名与预期、昵称与更新前一致
 → 最后复核当前账号，再返回 verified=true
```

不写 account 内存或 SelfInfo 缓存。昵称必须来自服务端读取；客户端界面可能自行稍后同步。
读取前后快照是尽力而为检查，不排除 ABA 账号切换或完整原生对象生命周期竞争。

## 签名字段证据

公开原始协议定义 [mm.pb.go](https://github.com/ThePeppy/wechat-api-ipad/blob/main/Cilent/mm/mm.pb.go)
的 ModUserInfo 声明 Signature 为 bytes,13，对应直接字符串，不是 SKBuiltinString_t 包装。
[UpdateProfile.go](https://github.com/ThePeppy/wechat-api-ipad/blob/main/models/User/UpdateProfile.go)
也将 Signature 按字符串写入并通过 Oplog CmdId=1 更新。该公开实现只提供协议交叉核对，
未直接照搬其整份资料构造逻辑，避免覆盖其它用户字段。

本项目对当前安装 DLL 独立核对：ModUserInfo 名称 RVA 0x918D0C8，
名称函数 0x101E540，虚表 0x918D068，序列化函数 0x101D370。
具体指令：0x101D6C1 读取对象+0x58，0x101D6C5 设置字段号 0xD，
0x101D6CD 调用原生字符串序列化 0x1618140。
本轮普通 pefile/Capstone 输出在 .review/signature-moduserinfo-*，不称为 REA Evidence。
字段语义另经真实改为 test 和恢复原签名的服务端回读验证。

缺失 field 13 按 proto2 optional string 的空默认值读取；更新时显式写入 field 13，
包括清空时的零长度字符串。重复 field 13、错误 wire 类型、截断 PB 均拒绝更新。
保留 ModUserInfo 其余字段和未知字段原始字节，Oplog 长度随实际字节重新生成。

## 构建、原生 ABI 和生命周期

仅适配已核验的 AMD64 Weixin.dll：timestamp 0x6ACCBF76，SizeOfImage 0xC34F000。
目标 C:\Program Files\Tencent\Weixin\4.1.15.55\Weixin.dll。
SHA256 6fbbba536b04c0a24730710f540887a53f4eaa24398e4f94905d9b4a36148595。

| 项目 | 当前核验的入口 / 布局 |
|---|---|
| GetProfile 提交 / 构造 | 0x49A1C0（CGI 302）/ 0x1009600（32 字节） |
| Oplog 提交 / 构造 | 0x19B7D40（CGI 681）/ 0x3B1B040（24 字节） |
| 当前账号 / CGI service | manager=*(base+0xBB92FC0)，service=*(manager+0x2C8)，service 虚表 0x8FCD388 |
| 部分解析 / 原生分配 | 0x697160 / 0x754C41C |
| 序列化 / 释放 | message 虚表+0x40、+0x50；request 自身的原生虚析构 |

沿用本项目新版昵称适配已核验的异步 submit(service, request, nativeWxidString, options, functionStorage)。
原生场景复制请求并接管回调，提交返回后释放本地原生 request。
回调通过 scene 虚表+0x20 取得响应，复制 PB 到自有缓冲区，不保留响应内部指针。
完成状态共享所有权和模块 pin 保证等待超时后的晚到回调代码存活；超时不取消原生请求。
没有扩大旧版 NativeCgi.h 的兼容范围；其它未适配接口仍不能借用这些入口运行。
更完整的调用约定与场景布局见 [新版昵称记录](edit-self-nickname-4.1.15.55.md)。

## 本轮验证

- tools/signature_test.cpp：wire 及生产 HTTP 注册函数测试通过。覆盖中文/空签名、
  field 13 缺失默认值、插入/替换、原始其它字段保留、Oplog 长度、重复/截断/错误类型、
  JSON 参数、NUL/非法 UTF-8/超长、非法输入不调用 backend、超时和禁止未验证成功。
- 原有昵称生产 HTTP 回归测试重新编译并通过。
- tools/signature_native_test.cpp 与 signature_native_runner.py：新版签名 Oplog 的原生
  构造、部分解析、序列化、析构往返通过。
- 当前账号的真实 GetProfile 签名读取通过；原签名字节长度为 0，日志不保存个人值。
- 微信内短时随机本机端口的生产 HTTP 测试通过：相同签名返回成功且 submitted=false，
  非法请求失败；持有共享锁时，签名和昵称两个生产路由均拒绝提交，证明当前 DLL 内锁互通。
- 根据本会话明确授权，真实将签名设为 test，再恢复原来的空签名。两次操作均
  ret=0、submitted=true、verified=true；独立 GetProfile 确认原签名恢复且昵称未改变。
- 修改前保存 Windows DPAPI 加密恢复数据；独立恢复确认后删除，recovery_retained=false。
  日志未保存明文签名、昵称、wxid、完整用户资料或会话凭据。临时 HTTP 服务已停止。
- GetSelfProfile.cpp、MsgExtended.cpp（共享写入锁）当前源码对象编译通过；
  Apifox JSON 解析、runner Python 语法及 git diff --check 通过。

真实结果摘要：edit_ret=0、edit_submitted=true、edit_verified=true；restore_ret=0、
restore_submitted=true、restore_verified=true；independent_restore_readback=true、nickname_preserved=true。

完整 Hook DLL 尚未构建替换部署；独立诊断 DLL 验证不能替代完整代理 DLL 部署验收。
其它构建、真实账号切换/退出重登、网络超时、晚到回调、高并发和 PC 界面同步未实测。
共享写入锁仅覆盖同一 DLL，不能协调已加载的其它诊断 DLL。

## 复现工具

```powershell
# 项目已有 Zig 0.16.0；global/local cache 应设在本项目 .review。
.review/compiler/ziglang/zig.exe c++ -std=c++17 -Iinclude tools/signature_test.cpp -o .review/signature_test.exe -lws2_32 -lcrypt32
.review/signature_test.exe
.review/compiler/ziglang/zig.exe c++ -shared -std=c++17 -Iinclude tools/signature_native_test.cpp -o .review/signature_native.dll -lws2_32 -lcrypt32
# 重新枚举实际加载 Weixin.dll 的当前主进程 PID；不复用历史 PID。
python tools/signature_native_runner.py --pid <当前主进程PID> --codec
python tools/signature_native_runner.py --pid <当前主进程PID>
python tools/signature_native_runner.py --pid <当前主进程PID> --http
# 会真实修改并恢复签名，必须先获得用户明确授权。
python tools/signature_native_runner.py --pid <当前主进程PID> --edit-restore
```

runner 将原生 worker 非零退出码作为失败退出；同名已加载 DLL 会被复用。
修改诊断源码后需使用新 DLL 文件名或在微信重启后再验证，不能用历史已加载模块证明新代码。
