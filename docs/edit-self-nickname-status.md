# EditSelfNickname 实现与验证

日期：2026-10-08。仅支持当前 Windows x64 Weixin 4.1.15.13 构建（PE 时间戳 0x6AB0A378，SizeOfImage 0xC0AE000）。

## 使用方式

POST `/EditSelfNickname`，JSON：`{"nickname":"test"}`。
兼容 `nick_name`；存在 `nickname` 时优先使用它。拒绝错误 JSON、非对象、非字符串、空值、NUL、无效 UTF-8，以及超过 4096 字节的输入；4096 是防御性上限，实际业务限制由服务端决定。

返回 `ret`、`retmsg`、`submitted`、`verified`。只有服务端资料读取确认昵称匹配才返回 `ret=0, verified=true`。原昵称已匹配时不重复提交。超时返回失败并明确结果未知，不自动重试。

## 生产路径

- `include/EditSelfNicknameRoute.h`：HTTP 注册、参数校验和返回值。
- `include/EditSelfNickname.h`：构建与登录校验、原生异步请求、回调生命周期、修改后服务端查询。串行处理本 DLL 的昵称修改请求。
- `include/NativeCgi.h`、`include/ProtobufWire.h`：后续文本发送实现中提取的共享原生回调与 wire 工具；昵称逻辑和外部参数保持原接口。
- `include/NicknameWire.h`：protobuf 编码、严格解析、业务返回校验。
- `src/MsgExtended.cpp`：注册生产路由；不写 SelfInfo 或 account+0x88 来模拟成功。

先 GetProfile 读取完整 ModUserInfo 并核对 wxid；只替换昵称字段，保留其它字段及未知字段；通过 Oplog CmdId=1 提交，再检查业务返回及 GetProfile 查询结果。协议结构另与[公开 UpdateProfile 源码](https://github.com/ThePeppy/wechat-api-ipad/blob/main/models/User/UpdateProfile.go)交叉核对，原生入口和 ABI 以本机 DLL 静态分析及实测为准。

## 已验证 RVA 与 ABI

| 项目 | RVA / 布局 |
|---|---|
| CGI 服务 | manager=*(base+0xB912690)，service=*(manager+0x2C8)，虚表 base+0x8DFA208 |
| GetProfile 异步 scene 提交 | 0x485240，CGI 302，URI /cgi-bin/micromsg-bin/getprofile |
| Oplog 异步 scene 提交 | 0x19A1010，CGI 681，URI /cgi-bin/micromsg-bin/oplog |
| GetProfileRequest 构造 | 0xFE5770，32 字节 |
| OplogRequest 构造 | 0x3A93040，24 字节 |
| protobuf 部分解析 / 序列化 | 0x681480 / 0x681AD0 |
| 原生分配 / sized free | 0x74AB89C / 0x74AB8E0 |
| native string | 32 字节；size+16、capacity+24、短字符串容量 15 |

提交 ABI：`submit(service, request, nativeWxidString, options, functionStorage)`。原生 scene 复制 request，接管 MSVC std::function 的 heap implementation，再经 service 虚表 +0x28 的锁保护注册及网络任务队列发送。回调为 `bool(implementation, scene**, errorType*, errorCode*)`；scene 虚表 +0x20 获取响应。函数存储 64 字节，implementation 位于 +0x38。回调对象由本 DLL 创建和销毁，不混用 C++ 标准库对象布局。

每次原生请求最多等待 30 秒，完整修改最多包含三次请求。超时后回调共享状态仍存活；模块 pin 到进程结束，防止晚到回调执行已卸载代码。用户登出、重登录和其它构建尚未实测；版本不匹配拒绝调用。

## 实测结果与边界

- protobuf 协议测试通过，包括中文、未知字段保留、账号不匹配、截断、重复字段和服务端错误。
- 原生 protobuf 构造、部分解析、序列化、销毁通过。
- 原生异步 GetProfile 收到完成回调，修改前服务端昵称与本地一致。
- 经用户授权将真实账号昵称改为 `test`：Oplog 业务成功，随后 GetProfile 确认 `test`，返回 submitted=true、verified=true、ret=0。
- 生产 HTTP 注册函数在微信进程中的独立诊断 DLL、仅监听 127.0.0.1:30089 的短时服务内实测：两个参数名均通过；错误 JSON、数组、缺少字段、数字、空字符串和 NUL 均返回失败。测试服务已停止。
- MsgExtended.cpp Windows x64 对象编译通过；诊断 DLL 链接通过。完整项目 MSVC DLL 尚未构建和替换部署，不能将诊断服务测试等同于完整项目部署。
- 客户端缓存不被直接写入，也没有强制界面刷新；本接口的 verified 指服务端资料查询。GetSelfProfile 读取客户端内存，可能在客户端自行同步前仍显示旧昵称。

调查期间同步 coroutine bridge 0x45FB50 从普通远程线程调用导致主进程异常退出，已重启恢复；当时未提交昵称修改。该入口及同类 Oplog coroutine bridge 未接入最终实现。最终采用已通过真实请求验证的异步 scene 提交。

## 复现工具

`tools/nickname_wire_test.cpp`：无网络协议测试。
`tools/nickname_native_test.cpp` 与 `tools/nickname_native_runner.py`：指定进程的原生查询、codec、授权修改 test，以及短时生产 HTTP 路由验证。runner 的 `--edit-test` 会真实修改账号昵称，仅在明确授权时使用。
`tools/nickname_analysis.py`、`tools/nickname_probe.py`：静态分析及只读地址诊断。静态分析依赖 pefile/capstone；工具输出不记录 wxid、手机号、令牌或完整用户资料。
