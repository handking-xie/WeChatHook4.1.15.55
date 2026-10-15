# SendTextMsg_NoSrc 实现与验证

日期：2026-10-08。仅适用于 Windows x64 Weixin 4.1.15.13，PE 时间戳
`0x6AB0A378`、SizeOfImage `0xC0AE000`。版本或登录检查失败时不提交。

## 请求与成功判定

POST `/SendTextMsg_NoSrc`：

```json
{"toid":"filehelper","msg":"nosrc_myloveisverygood_haha cc加油"}
```

收件人兼容 `toid`、`wxid`、`wxidorgid`、`gid`，按此顺序取首个非空字符串；
消息兼容 `msg`、`content`。选取过程中遇到错误字段类型立即拒绝。
拒绝错误 JSON、非对象、空目标、空文本、无效 UTF-8、NUL，以及超过防御性上限的输入
（目标 4096 字节，文本 1 MiB）；实际业务限制由微信服务端判断。

返回 `ret`、`retmsg`、`submitted`、`verified`。进入提交阶段后包含
`client_msg_id`，服务端业务确认成功后包含十进制字符串 `msg_id`，避免大整数精度损失。
`ret=0, verified=true` 表示服务端确认发送，且返回的收件人、客户端消息 ID 匹配，
服务端消息 ID 非零；不表示对方已读或客户端界面已同步。
等待完成回调最多 30 秒；超时表示结果未知，不自动重试。

原实现经 `SafeSendTextRaw` 调用不抛异常就返回成功，不能证明发送完成。
此次仅替换 `/SendTextMsg_NoSrc`；`/SendTextMsg`、`/SendAtMsg_NoSrc` 保持原路径。

## 代码与原生证据

- `include/SendTextNoSrcRoute.h`：生产 HTTP 注册、参数校验、响应。
- `include/SendTextNative.h`：版本和登录检查、随机客户端消息 ID、异步发送和业务校验。
- `include/SendTextWire.h`：单条文本请求与严格的响应关联校验。
- `include/NativeCgi.h`：从昵称实现提取的原生异步 scene、MSVC ABI 回调和所有权处理。
- `include/ProtobufWire.h`：共享 protobuf wire 编解码。NicknameWire 保留原接口。
- `src/SendTextMsg.cpp`：注册新 NoSrc 路由。

以下 RVA 经本机目标 DLL 静态分析确认：

| 项目 | RVA / 布局 |
|---|---|
| SendMsgRequestNew 构造 | `0x2E82760`，48 字节 |
| 请求 protobuf 虚表 | `0x93D4EC8` |
| 异步 scene 提交 | `0x39F28C0`，CGI 522，URI `/cgi-bin/micromsg-bin/newsendmsg` |
| scene 构造 / 虚表 | `0x39F2C00` / `0x95149B8` |
| scene 中请求 / 响应 | `+0xA8` / `+0xD8` |
| 响应 getter | `0x10E3A0`，scene 虚表 `+0x20` |
| 原生回调 invoke | `0x39F2EE0`，参数 ABI 与共享 NativeCgi 回调一致 |

提交 ABI 与昵称实例相同，但请求构造、大小、提交入口和业务协议分别核验。
原生 scene 复制请求、接管函数存储中的 heap implementation，经 CGI 服务锁保护注册
和原生网络队列提交。同步 coroutine wrapper `0x39EDFD0` 不用于本实现。

SendMsgRequestNew 外层字段 1 为 MsgCount=1，字段 2 为单条 MsgRequest。
内层字段：1 收件人 SKBuiltinString_t；2 文本；3 Type=1；4 Unix 秒时间；
5 随机 uint32 ClientMsgId；6 MsgSource 空字符串；7 CtrlFlag=0。
NoSrc 不生成转发来源或 @ 列表。随机数来自 BCryptGenRandom，不能当成成功凭证。

SendMsgResponseNew 字段 1 为 BaseResponse，2 为 Count，3 为结果列表。
检查 BaseResponse.Ret、单项 Ret 均为 0、Count=1、仅一项，单项收件人及 ClientMsgId
与请求一致，并读取字段 8 的 uint64 服务端消息 ID；缺失、重复、截断或不匹配均失败。

回调中复制响应后再通知等待者；超时后共享状态继续由原生回调持有。
承载回调的 DLL pin 到进程结束，不在晚到回调前卸载。
本路径直接提交服务端请求，不伪造本地数据库消息或界面记录；客户端缓存的同步另由微信处理。

## 验证结果

- 协议测试通过：UTF-8 文本、空来源、服务端大整数 ID、错误收件人、错误 ClientMsgId、
  重复结果、BaseResponse/单项拒绝和截断输入。
- 提取共享 protobuf 工具后，昵称协议测试继续通过。
- 原生 codec 实测通过：请求解析成功、IsInitialized=true、序列化与输入完全一致、对象释放正常。
- SendTextMsg.cpp 编译为 Windows x64 对象成功，独立诊断 DLL 链接成功。
- 微信进程内短时 localhost HTTP 服务复用生产注册函数，错误 JSON、数组、缺字段、
  错误字段类型、空文本和 NUL 均返回失败；未登录时 submitted=false。
- 用户明确指定 filehelper 和上述文本后，生产 HTTP 路由真实发送成功：
  ret=0、submitted=true、verified=true，服务端消息 ID `1328595156538976503`。
  服务端返回的收件人、ClientMsgId 均与请求匹配；只提交了一条真实消息。
  登录前及启动器进程中的有效请求被账号检查拦截，submitted=false。
- 完整项目 MSVC DLL 尚未构建和替换部署；其它构建、群聊、登出竞争和超时未实测。

复现：`tools/sendtext_wire_test.cpp` 是无网络协议测试；
`tools/sendtext_native_test.cpp` 导出 RunCodec（不发送）和 RunHttp（45 秒本机服务）。
可用 `tools/nickname_native_runner.py --dll sendtext_native_test.dll --codec --result-file sendtext-native-result.json`
指定当前微信主进程运行 codec；`--http` 启动端口 30089。
向测试路由提交有效请求会真实发送消息，只能使用用户已明确授权的目标和内容。
