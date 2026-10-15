# GetFriendTimeline 原生网络实现记录

日期：2026-10-08。

`POST /GetFriendTimeline` 与 `POST /mmsnsuserpage` 现在通过微信原生
`mmsnsuserpage` CGI 读取服务端个人相册，等待完成回调并校验业务结果。
两个路由已切换到网络路径；其它 Timeline 首页/下一页路由仍读本地缓存。
网络失败不回退缓存，不自动重试，不主动写入 sns.db 或强制刷新微信界面。

## 请求与响应

```json
{"wxid":"wxid_target","count":20,"id":""}
```

- 用户必填，按 `user_name`、`username`、`wxid` 顺序选择首个非空字符串。
- 首页省略 `id` 或传空字符串/`"0"`；下一页传响应的 `next_id`。
  网络游标为十进制 uint64 朋友圈 ID 字符串，不再接受本地数据库 tid 或任意 XML 游标。
- `count` 为整数，默认 20，非正数用 20，最大 100。服务端控制单次页大小，
  count 仅限制本次返回条数，不保证拉满。
- 错误 JSON、非对象、错误字段类型、缺少用户、NUL、超长字符串、非法 UTF-8、
  非十进制或溢出的游标均在提交前拒绝。4096 字节为本地防御性字符串上限。

普通成功返回 `ret=0`、`retmsg=success`、`submitted=true`、`verified=true`、
`source=network`、`user_name`、`count`、`server_count`、`data`，非空时返回 `next_id`。
已提交请求保留 `business_ret`；成功响应增加 `visibility_limited`。
业务码 207 表示已核验的可见时间窗口受限状态，返回 ret=0、verified=true、
visibility_limited=true，retmsg 使用服务端字段 12 的原文，同时保留 server_message。
例如本次实际提示“朋友仅展示最近三天的朋友圈”，返回 0 条；不能据此判断对方从未发布动态。
其余未核验的非零业务码仍失败，207 也必须通过数量、对象和账号一致性检查。
每条记录含字符串 sns_id、user_name、字符串 create_time、nickname、原始 XML content，
可提取时补充 content_desc（去除完整 CDATA 包装）；不编造数据库 tid。
XML 标签提取不是完整 XML 解析器。

先按 ID 降序选取 count 条，再按时间稳定降序展示。
next_id 为选中记录最小 ID，应直接使用它翻页，不能用展示数组末条 ID。
这样避免截断服务端页或展示排序导致跳过记录；不保证跨页整体时间降序。

失败返回 ret=-1、具体 retmsg、空 data 和真实提交/验证状态；
提交后附带 business_ret，传输失败或未解析时为 -1，业务拒绝时为实际服务端码。
verified 表示已取得并校验服务端响应，不代表客户端缓存或界面已同步。
空相册可成功返回空数组；好友可见范围由服务端决定。

## 构建与独立核验入口

仅支持 Windows x64 Weixin 4.1.15.13 已核验构建：PE 时间戳 `0x6AB0A378`，
SizeOfImage `0xC0AE000`。调用前用 ProfileMemory 校验版本、当前账号和登录状态；
完成后再核对账号未改变。

| 项目 | RVA / 布局 |
|---|---|
| SnsUserPageRequest 构造 / 大小 / 虚表 | 0x5522F50 / 0x58 / 0x9813AB8 |
| 请求解析 / 序列化 | 0x554C7E0 / 0x554CED0 |
| 异步提交 / CGI / URI | 0x3018880 / 212 / /cgi-bin/micromsg-bin/mmsnsuserpage |
| scene 构造 / 大小 / 虚表 | 0x3018BD0 / 0x1C0 / 0x93FE038 |
| scene 请求 / 响应成员 | +0xA8 / +0x100 |
| 响应 getter（虚表 +0x20） | 0x105BE10，返回 this+0x100 |
| scene BaseRequest 准备 | 0x3018EE0 |
| SnsUserPageResponse 虚表 / 序列化 | 0x9813BD8 / 0x554F510 |
| SnsObject 虚表 / 序列化 | 0x9812A28 / 0x5533580 |
| 客户端相册请求构造 | 0x2FE4950 |

由 URI、协议名称引用、请求/响应虚表及 scene 构造交叉定位。
异步提交参数为 `(service, request, nativeString, options, functionStorage)`。
提交函数复制请求、转移 std::function implementation，经 service 虚表 +0x28 注册 scene，
再经 0x67C180 / 0x67C520 调度到原生队列。
响应 getter 和回调转移 ABI 经独立核验后复用 NativeCgi 的完成状态、所有权、
30 秒等待和 DLL pin。不调用需要 coroutine 上下文的同步包装 0x3015C40。

## 协议与成功条件

客户端 0x2FE4950 在请求 +0x10 写空摘要，在 +0x18 复制用户名；
结合原生序列化函数确认字段号，不能仅凭类型相同推断语义。

| 请求字段 | 内容 |
|---|---|
| 1 | BaseRequest，由 scene 补充 |
| 2 | FirstPageMd5 空字符串，要求完整响应 |
| 3 | 目标用户名 |
| 4 | MaxId：首页 0，下一页为游标 |
| 5、6、7、9 | 对应客户端初始相册请求的零值，不开放未核验的筛选参数 |

响应字段 1 是 BaseResponse，字段 3 是数量，字段 4 是重复 SnsObject。
接受 BaseResponse.Ret=0 或已核验的 207 可见范围状态；数量与对象数必须一致，
核心字段无重复或截断。响应字段 12 为客户端使用的显示提示（+0x60），
有该字段时验证唯一字符串、UTF-8、NUL 和 4096 字节上限后返回 server_message。
SnsObject 字段 1/2/3/4/5 分别为 ID、用户名、昵称、时间、ObjectDesc。
ObjectDesc 为 SKBuiltinBuffer_t，字段 1 长度须与字段 2 XML 字节数一致。
每条对象必须属于请求用户，ID 非零且不重复，下一页 ID 小于 MaxId。
XML 和昵称需为有效 UTF-8 且无 NUL；失败不发布部分结果。
共享原生字符串读取对单个响应限制为 1 MiB，超过防御性上限时失败。

## 生产代码与验证

- include/FriendTimelineWire.h：编码、严格解析、数字游标。
- include/FriendTimelineNative.h：提交、业务确认、账号一致性校验。
- include/FriendTimelineRoute.h：生产注册、响应转换、排序和分页。
- src/Timeline.cpp：注册网络路由，保留其它缓存接口。
- tools/friend_timeline_wire_test.cpp、tools/friend_timeline_route_test.cpp：
  协议错误、用户关联、大整数、数量、截断页、CDATA 和提交状态测试。
- tools/friend_timeline_native_test.cpp：原生 codec、当前账号相册及短时 HTTP 服务。
- tools/friend_timeline_http_test.py：从既有缓存选择一名用户进行只读网络路由验证，
  仅输出数量和检查状态，不保存用户名或动态内容。

Windows x64 Zig 0.16.0 编译并运行参数、协议和路由测试通过；Timeline.cpp 对象编译、
诊断 DLL 链接、SQLite 查询回归与 git diff --check 通过。

在已核验且登录的微信进程内：

1. 原生请求解析、序列化回读完全匹配、释放正常。
   提交前缺少 BaseRequest，IsInitialized=false；scene 补充该字段。
2. 初版误将用户名放入字段 2，真实服务端返回 Ret=-2，未报告成功。
   对照客户端构造修正为字段 3，并重新通过 codec 验证。
3. 修正后当前账号相册网络请求返回业务码 0、10 条对象、submitted=true、verified=true。
4. 修正后的生产注册函数在 45 秒本机诊断服务中，指定好友首页返回 3 条，
   下一页返回 3 条，各页时间降序、无重复；兼容路由返回 1 条，均 source=network。
   错误 JSON、数组、缺字段、错误类型、NUL、浮点 count 和溢出游标均提交前失败。
   短时诊断服务已停止。

完整 MSVC 项目 DLL 尚未构建和替换部署；诊断服务实测不等于完整项目部署验证。
其它构建、真实账号切换、超时后的晚到回调、好友权限变化和所有筛选模式未实测。

## 207 可见范围状态修正

原实现将所有非零业务码视为拒绝，丢失字段 12 的提示。
客户端相册处理 0x2FE6A41 对 207 单独分类，在 0x2FE6A51 设置相册状态 1，
并读取响应 +0x60 的文本；不会把它当作正常无限制状态 0。
对用户给定目标的只读诊断确认 Ret=207、ObjectCount=0、实际对象数=0，
服务端提示为“朋友仅展示最近三天的朋友圈”。不在文档保存目标账号。

现将 207 作为有效的受限范围响应处理，保留真实业务码和服务端文本，
不推断或绕过好友的可见设置；不把其它非零码一并视为成功。
协议测试覆盖 207 空页、可见对象、数量错误、重复提示、未核验的 208 和业务码溢出；
路由测试覆盖业务码/提示保留及空页无 next_id，均通过。
生产 Native Fetch 和两个生产 HTTP 注册路由用该目标实测通过：
ret=0、business_ret=207、verified=true、visibility_limited=true、count=0。
普通业务码 0 及分页行为的协议/路由回归通过；源文件对象编译与诊断 DLL 链接通过。
额外普通好友 HTTP 回归在短时服务停止后运行，连接被拒绝，该次运行未完成。
完整项目 DLL 尚未替换部署，运行服务需重建并加载更新后生效。
