# sns_like 原生实现记录

接口：`POST /sns_like`。请求必须包含字符串 `sns_id`（非零 uint64 十进制）和 `to_wxid`（朋友圈作者）。

```json
{"sns_id":"15027695064333161080","to_wxid":"wxid_0xexx8mmtng912"}
```

实现分为 SnsLikeWire.h、SnsLikeNative.h、SnsLikeRoute.h，由 Route_Timeline 注册。
校验 JSON 对象、必填字符串、十进制范围、空值、NUL、UTF-8 和目标 4096 字节上限。
ID 使用字符串避免 JavaScript 精度丢失。

仅支持 Weixin 4.1.15.13 的 PE 时间戳 0x6AB0A378、SizeOfImage 0xC0AE000，复用 ProfileMemory 的构建及登录校验。

| 入口 | RVA / 布局 |
|---|---|
| SnsCommentRequest 构造 | 0x5523190，0x28 字节，虚表 0x9813FA8 |
| 异步提交 | 0x301E860，CGI 213，/cgi-bin/micromsg-bin/mmsnscomment |
| scene 构造 | 0x301EBB0，0xF0 字节，虚表 0x93FEC78 |
| 请求 / 响应 | +0xA8 / +0xD0 |
| 响应 getter | 0x789780 |
| SnsAction 构造 / 序列化 | 0x55230C0 / 0x55539A0 |
| SnsActionGroup 序列化 | 0x5554F90 |
| SnsCommentRequest 序列化 | 0x5555B80 |

静态依据：URI 引用 0x301E8DF，提交函数复制请求、转移 MSVC std::function 的 +0x38 implementation，经 CGI 服务虚表 +0x28 注册并由 0x67C180 / 0x67C520 调度。
scene getter 返回 this+0xD0。客户端 0x2FE91E0 中构造 SnsActionGroup 的字段 1 ID、字段 3 CurrentAction；动作字段 1 当前账号、2 作者、5 Type、6 Source、7 CreateTime。Type 1 为添加点赞，5 为移除。
当前请求使用 Source=0、Unix 秒时间；客户端 Source 来自业务对象 +0x244，此零值的实际业务兼容性需实测，不宣称与所有客户端场景完全等价。
外层字段 2 为 ActionGroup，BaseRequest 由 scene 补充；不开放取消点赞或其它未经验证的动作。

等待异步完成最多 30 秒，回调复制响应后再解析。仅传输成功、BaseResponse.Ret=0、响应字段 2 SnsObject 的字段 1 ID 匹配才返回 ret=0、verified=true。
submitted=true 表示已进入原生提交阶段。超时表示结果未知，不自动重试；DLL pin 及晚到回调生命周期复用 NativeCgi。
操作串行化仅约束本 DLL 内调用，超时不会取消原生任务。接口不修改本地数据库，也不强制刷新界面。

验证记录（2026-10-09）：

- Zig 0.16.0 Windows x64 协议及路由测试通过：大整数 ID、动作类型、业务拒绝、响应 ID 不匹配、缺失/重复对象、截断、错误参数、溢出、NUL、非法 UTF-8、长度限制和提交后超时状态。
- Timeline.cpp Windows x64 对象编译通过；原生诊断 DLL 链接通过；git diff --check 通过。
- 进程内 codec 尝试在创建模块快照时遇到 WinError 299；随后进程列表确认微信进程已不存在。诊断 DLL 未加载，未发送点赞，codec 和真实业务请求均未完成验证。
- 完整 MSVC DLL 未构建、未替换部署，现有服务需重建并加载新 DLL。其它构建、Source=0 在其它动态上的服务端兼容性、账号切换、晚到回调和界面同步未验证。

诊断命令（先重新核验登录进程 PID）：

```text
python tools/sns_like_native_runner.py --pid <PID> --codec
python tools/sns_like_native_runner.py --pid <PID> --like
```

`--like` 会对请求示例中的指定朋友圈真实点赞一次；必须显式选择该参数，不作为默认测试。结果写入 .review/sns-like-native-result.json。

## 登录后真实点赞验证（2026-10-09）

重新定位已登录主进程，并通过生产 ProfileMemory 校验模块构建与账号状态。
原生 SnsCommentRequest codec 实测 parsed=true、roundtrip_matches=true，原生对象正常释放。
随后经用户明确授权，通过诊断 DLL 调用生产 SnsLikeRoute::Handle 和 SnsLikeNative::Like，对示例中指定动态提交一次点赞。
返回 ret=0、business_ret=0、submitted=true、verified=true；生产响应校验确认 SnsObject ID 与指定 SNS ID 匹配，回调后账号一致性检查通过。
此结果确认指定动态的服务端业务成功及 Source=0 在本次请求中的兼容性。不代表客户端界面已同步，也不能推广到其它动态或构建。
本次未重复提交、未取消点赞。未运行完整项目 HTTP 服务或替换部署 DLL，因此生产 Handle 的进程内测试不能替代完整 DLL HTTP 端到端验证。
