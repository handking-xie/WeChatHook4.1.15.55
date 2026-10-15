# 原生接口实现参考：资料读取、昵称修改、文本发送与朋友圈分页

保存日期：2026-10-08。适用于本项目后续微信接口的定位、实现与验证。

## 参考代码与证据

- `include/ProfileMemory.h`：版本校验、安全内存读取、字符串解析和一致性检查。
- `src/GetSelfProfile.cpp`：将原生读取结果转换为 HTTP 响应。
- `tools/profile_memory_test.cpp`：复用生产读取函数的模拟测试与进程实测工具。
- [4.1.15.13 偏移与验证记录](self-profile-offsets-4.1.15.13.md)：当前已发现的指针链、字段依据和验证范围。
- `include/EditSelfNickname.h`：原生异步 scene 提交、完成回调及服务端回读。
- `include/EditSelfNicknameRoute.h`、`include/NicknameWire.h`：HTTP 参数处理、protobuf 编码和业务返回校验。
- `tools/nickname_wire_test.cpp`、`tools/nickname_native_test.cpp`、`tools/nickname_native_runner.py`：协议测试、原生调用和生产路由诊断。
- [EditSelfNickname 实现与验证](edit-self-nickname-status.md)：版本专属入口、ABI、真实修改结果和验证边界。
- `include/NativeCgi.h`、`include/ProtobufWire.h`：昵称与文本发送共用的异步回调 ABI、生命周期及协议工具。
- [SendTextMsg_NoSrc 实现与验证](send-text-nosrc-4.1.15.13.md)：独立核验的文本请求、空来源和服务端消息关联结果。
- `include/SendTextNoSrcRoute.h`、`include/SendTextNative.h`、`include/SendTextWire.h`：文本发送的 HTTP、原生调用及业务协议层。
- `tools/sendtext_wire_test.cpp`、`tools/sendtext_native_test.cpp`：无网络协议测试、原生 codec 与生产路由诊断。
- `include/Timeline.h`、`src/Timeline.cpp`：朋友圈首页、下一页的路由注册、本地数据库查询、XML 字段提取和返回排序。
- `include/FriendTimelineWire.h`、`include/FriendTimelineNative.h`、`include/FriendTimelineRoute.h`：
  指定好友相册的原生网络协议、异步提交、业务状态校验和生产 HTTP 路由。
- `tools/friend_timeline_wire_test.cpp`、`tools/friend_timeline_route_test.cpp`、
  `tools/friend_timeline_native_test.cpp`、`tools/friend_timeline_http_test.py`：
  协议与路由测试、微信进程内 codec/只读网络诊断和 HTTP 分页验证。
- [GetFriendTimeline 原生网络实现记录](get-friend-timeline-status.md)：独立核验的入口、协议映射、
  真实网络结果和业务码 207 的可见范围状态处理。

后续接口可以复用调查和验证方法，不能直接复用账号对象偏移、登录标志或虚表地址。
这些数值是当前接口、当前 DLL 构建的证据，不是通用微信布局。

## 实现目标

接口返回真实的原生读取结果或真实操作结果。不要用参数回显、随机值、时间戳、
虚构标识符或固定成功响应替代实现。未确认的字段和未实现的操作必须明确说明。

GetSelfProfile 当前采用纯偏移读取：生产路径不查询数据库，不解析账号目录，
不全内存扫描，也不读取全局 SelfInfo 缓存。定位期间使用旧接口输出作为搜索线索，
这不构成生产依赖，也不能单独证明对象身份。

## 定位流程

1. 确认主进程和实际加载的目标模块，记录文件版本、架构、模块基址、
   PE 时间戳和 SizeOfImage。文件夹名、README 和分支名不能替代实际模块信息。
2. 收集最少的已知字段作为搜索线索。搜索结果可能来自日志、消息、联系人、
   缓存或历史账号；不要把第一次命中的字符串地址当作当前账号结构。
3. 检查候选字符串对象的长度、容量和周边字段，寻找结构一致的对象。
   字符串内容相同只能作为线索；字段类型和语义应分别验证。
4. 找到模块全局变量到对象的指针链，使用模块相对地址保存根入口。
   不把本次进程的绝对堆地址写进生产代码。
5. 分析目标对象的虚表、访问函数和字段访问指令，交叉验证指针链与字段位置。
   对空字段不能仅凭相邻位置推断它是邮箱、地区或签名。
6. 用独立读取程序重新沿该链获取数据，再与已知信息或账号界面核对。
   定位工具只读进程，不通过修改内存来让候选对象满足预期。

GetSelfProfile 的实例：

```text
manager = *(Weixin.dll 基址 + 0xB912690)
account = *(manager + 0x68)
校验 *(account) == Weixin.dll 基址 + 0x8DFE2F8
校验 *(BYTE*)(account + 0x38) 的 bit 0
```

| 字段 | 相对 account 的位置 | 当前依据 |
|---|---|---|
| wxid | +0x48 | 原生 getter 指令及内存值匹配 |
| 微信号 | +0x68 | 原生 getter 指令及内存值匹配 |
| 昵称 | +0x88 | 相邻字符串对象与已知昵称匹配 |
| 手机号 | +0xA8 | 相邻对象包含 11 位号码；仍需与账号设置独立核对 |

## 读取接口的代码结构

将原生读取与 HTTP 层分开，使独立测试能够调用同一个生产函数。

```text
HTTP 参数处理
  → 检查模块构建和对象布局
  → 安全读取到局部临时结果
  → 检查状态、字段格式与读取一致性
  → 成功后发布结果；失败返回具体错误
```

- 函数入口清空输出。任何失败都不发布半份数据或上次账号的结果。
- 固定偏移应用前检查模块身份。当前示例使用 PE 时间戳和映像大小，
  它们是兼容性检查，不是密码学完整性验证。
- 使用 ReadProcessMemory 检查实际读取字节数，避免裸指针解引用导致崩溃。
- 先读字符串描述符并校验长度、容量，再读取内容；根据实际布局区分短字符串和堆字符串。
  本例为 32 字节 MSVC 字符串，容量 15 使用内联存储，但其他接口的字段不一定使用该布局。
- 校验 UTF-8 和长度上限，避免损坏内容进入 JSON 序列化。
- 读取后再次检查根指针、对象指针、状态和字段。双次读取提供尽力而为的一致性检查，
  不等于持有微信内部锁，也不能证明对象的完整生命周期已被同步保护。
- HTTP 层只负责参数、序列化和明确的错误响应，不承担对象定位或数据猜测。

## TimelineGetFristPage 与 TimelineGetNextPage 的实现

两个接口均为 HTTP POST，在 `Route_Timeline` 中注册，共用
`HandleGetTimeline(req, res, isNextPage)`：首页传 `false`，下一页传 `true`。
首页另有兼容拼写 `/TimelineGetFirstPage`。当前实现读取微信进程内已加载的
`sns.db` 本地缓存，不调用朋友圈网络 scene，也不主动拉取服务端最新动态。
因此“最新在前”指本次查询返回的缓存记录按发布时间排序。

### 参数、数据来源与分页

| 参数 | 当前实现 |
|---|---|
| `count` | 默认 20；小于等于 0 时用 20，超过 100 时限制为 100 |
| `id` | 下一页游标；使用上一页响应的 `next_id`，支持数据库 `tid` 或 XML 中的 `id` |
| `user_name` / `username` / `wxid` | 可选个人相册筛选，按上述嵌套默认值顺序读取；未指定用户时读取好友动态流 |

好友动态流查询 `SnsMainTimeLineBreakFlag`，个人相册查询
`SnsUserTimeLineBreakFlagV2` 并按 `user_name` 筛选。两者均通过 `tid`
与 `SnsTimeLine` 连接，读取 `user_name` 和 `content` XML。
数据库查询按 `tid_heigh_bit DESC, tid_low_bit DESC` 选取最多 `count` 条记录。
下一页先定位游标的高低位，再选取高位更小，或高位相同且低位更小的记录。
`isNextPage=true` 但 `id` 为空时走首页查询；非空游标找不到时查询结果为空。

```text
HTTP 参数读取
  → 检查 sns.db 已加载
  → 根据用户筛选、ID 游标和 count 查询本页缓存
  → 从 content XML 提取字段，并按查询顺序记录 lastId
  → 将本页 data 按 create_time 数值稳定降序排序
  → 返回 data、count 和原查询顺序生成的 next_id
```

### 返回字段与时间排序

每条记录保留 `tid`、`user_name`、原始 `content`；XML 标签存在且非空时，
补充 `sns_id`（`id`）、`create_time`（`createTime`）、
`content_desc`（`contentDesc`）和 `nickname`。`ExtractXmlTag` 通过标签字符串
查找提取内容，不是完整 XML 解析器。`create_time` 仍以字符串返回。

返回前用 `std::stoll` 将 `create_time` 转成 `long long`，通过
`std::stable_sort` 按数值降序排列，最新发布的记录在本页最前面；时间相同的
记录保留原查询顺序。缺失时间或转换抛异常时，排序值按 0 处理。
排序实现位于 `src/Timeline.cpp`，不改变字段类型、查询条件或分页逻辑。
早期共享处理函数也用于 `/GetFriendTimeline` 和 `/mmsnsuserpage`；这两个路由现已切换
到独立原生网络路径，详见文末补充及 [GetFriendTimeline 实现记录](get-friend-timeline-status.md)。

`next_id` 在排序前按原始查询顺序生成，优先取最后一条记录的 XML `id`，
否则取其 `tid`。调用方必须使用响应的 `next_id`，不能自行取排序后数组末尾的
ID 作为游标。当前变更只保证每页内部按时间降序；分页仍按 ID 高低位进行，
若 ID 顺序与发布时间不一致，不保证跨页整体时间降序，也不改变本页入选记录。

成功返回 `ret=0`、`retmsg="success"`、`count`、`data`，有非空游标时附带
`next_id`；无更多缓存记录时可成功返回空数组。数据库未加载或查询失败时返回
`ret=-1`、具体 `retmsg` 和空 `data`。成功表示本地查询成功，不代表网络刷新完成。

### 验证记录与边界（2026-10-08）

- 修改排序前，对本机 `127.0.0.1:30001` 的已运行服务做过只读验证：
  首页请求 `count=5` 返回 5 条，使用其 `next_id` 请求下一页返回 5 条；
  两页各自及连接后的 `create_time` 均降序，两页无重复，两个接口均 `ret=0`。
- 首页请求 `count=100` 时，当前缓存返回 20 条且时间降序；使用末条游标继续请求
  返回空数组。该结果仅描述测试时缓存，不代表服务端没有更多动态。
- 随后源码增加了按 `create_time` 的稳定降序排序，`git diff --check` 通过；
  更新后的源码尚未编译、构建或部署，也未完成新 DLL 的 HTTP 端到端验证。
  既有服务的实测不能替代新排序代码的运行验证。
- 个人相册筛选、时间缺失或异常、相同时间、ID 与时间顺序不一致、缓存刷新、
  账号切换及其它微信构建尚未实测。此实现没有可供复用的朋友圈网络入口 RVA 或 ABI 证据。

## 操作接口需要额外验证

好友、群管理、消息发送等操作不能仅套用读取接口的模板。
必须确认函数入口、调用约定、线程要求、参数结构、内存所有权及异步回调生命周期。

“函数没有异常”不等于“操作成功”。如果原生调用只是提交任务，
应明确返回提交状态，并通过真实业务返回值、完成回调或可核验的状态变化判断完成。
不能把本地数据库写入解释为服务端好友、群成员或群主已变更。

### EditSelfNickname 的操作流程

当前实例仅支持 Windows x64 Weixin 4.1.15.13 的已核验构建：
PE 时间戳 `0x6AB0A378`，SizeOfImage `0xC0AE000`。
读取接口与操作接口都先检查构建和登录状态，但操作接口还需要真实提交与完成证据。

```text
HTTP 参数校验
  → 检查模块构建、登录状态，读取当前 wxid
  → 原生异步 GetProfile，核对服务端返回的账号身份
  → 昵称已匹配：直接返回 verified=true，不重复修改
  → 再次检查当前账号，保留 ModUserInfo 的其它字段，仅替换昵称
  → Oplog CmdId=1 提交修改，等待完成回调
  → 校验传输结果及 Oplog 业务返回
  → 再次 GetProfile，确认服务端昵称匹配后返回成功
```

此流程可以作为“读取现状—修改—业务确认—回读”的参考，具体请求结构和成功条件
仍需按接口重新验证。服务端资料已确认不等于客户端缓存或界面已经同步；
GetSelfProfile 读取本地内存，可能在微信自行同步前仍显示旧昵称。

### SendTextMsg_NoSrc 的操作流程

文本发送采用“构造请求—异步提交—业务确认—关联校验”，不套用昵称修改的资料回读。
原实现通过 `SafeSendTextRaw` 调用未抛异常就返回成功，缺少业务完成证据。
当前 `/SendTextMsg_NoSrc` 使用独立生产路径；`/SendTextMsg` 和 `/SendAtMsg_NoSrc`
仍使用原路径，不能将本次验证扩展到它们。

```text
HTTP 参数校验
  → 检查已核验的模块构建及当前账号登录状态
  → 生成随机 ClientMsgId 和 Unix 秒时间，构造一条文本请求
  → 原生异步 newsendmsg 提交，等待完成回调（最多 30 秒）
  → 检查 errorType/errorCode、BaseResponse.Ret、单项 Ret
  → 核对结果数量、收件人、ClientMsgId 和非零服务端消息 ID
  → 返回服务端确认结果；超时或关联失败返回具体错误
```

收件人按 `toid`、`wxid`、`wxidorgid`、`gid` 顺序取首个非空字符串，
消息按 `msg`、`content` 顺序选取；选取过程中遇到错误字段类型立即拒绝。
目标和文本必须非空、无 NUL 且为有效 UTF-8。4096 字节目标上限、1 MiB 文本上限
是本地防御性限制，不能解释为微信服务端业务限制。

#### 文本请求与原生入口

下表只适用于已核验的 Windows x64 Weixin 4.1.15.13 构建。

| 项目 | 已核验入口或布局 |
|---|---|
| SendMsgRequestNew 构造 | RVA `0x2E82760`，48 字节，请求虚表 `0x93D4EC8` |
| 异步 scene 提交 | RVA `0x39F28C0`，CGI 522，URI `/cgi-bin/micromsg-bin/newsendmsg` |
| scene 构造与虚表 | RVA `0x39F2C00`、`0x95149B8` |
| scene 请求与响应成员 | `+0xA8`、`+0xD8` |
| 响应 getter / 原生回调 invoke | RVA `0x10E3A0` / `0x39F2EE0` |

通过协议名称、URI 引用、protobuf 虚表及构造/解析/序列化函数交叉定位入口。
异步提交与回调 ABI 分别核验后，才复用 `NativeCgi`；不能仅凭函数形状相似就调用。
同步 coroutine wrapper `0x39EDFD0` 不用于此路径。

SendMsgRequestNew 外层字段 1 为 MsgCount=1，字段 2 为单条 MsgRequest。
内层布局如下：

| 字段号 | 内容 |
|---|---|
| 1 | 收件人 SKBuiltinString_t，嵌套字段 1 为目标字符串 |
| 2 | UTF-8 文本 |
| 3 | Type=1 |
| 4 | Unix 秒时间 |
| 5 | uint32 ClientMsgId，来自 BCryptGenRandom |
| 6 | MsgSource 空字符串 |
| 7 | CtrlFlag=0 |

NoSrc 不构造转发来源或 @ 列表。时间和随机 ID 只用于请求及响应关联，
不能作为发送成功证据。原生 codec 验证不仅检查解析成功，还确认
`IsInitialized=true`、序列化回读与输入完全一致，以及原生对象释放正常。

#### 文本发送的成功语义

SendMsgResponseNew 字段 1 是 BaseResponse，字段 2 是 Count，字段 3 是结果列表。
要求传输返回成功、BaseResponse.Ret=0、Count=1、仅一个结果，且该项 Ret=0。
单项字段 2 的收件人及字段 4 的 ClientMsgId 必须匹配请求，字段 8 的 uint64
服务端消息 ID 必须非零。缺失、重复、截断、业务拒绝或关联不匹配均返回失败。

HTTP 返回 `ret`、`retmsg`、`submitted`、`verified`；进入提交阶段后返回
`client_msg_id`，业务确认后返回十进制字符串 `msg_id`，避免 JSON 使用方丢失大整数精度。
`verified=true` 在发送接口中指服务端确认，不代表对方已读，也不代表本地界面已显示。
此路径不创建本地数据库或界面消息记录，客户端同步交由微信处理。

发送超时后可能仍已产生外部效果，因此不自动重试。昵称相同可以跳过修改，
相同文本却可能是用户有意重复发送；不能复用昵称接口的“值相同即成功”逻辑，
也不能把随机 ClientMsgId 宣称为已验证的跨请求幂等保证。

### 共享组件的复用范围

`NativeCgi.h` 从昵称实现提取了原生字符串、protobuf 部分解析/序列化、
scene 提交、MSVC std::function 回调适配及共享完成状态。
`ProtobufWire.h` 提供字段编码、严格解析和单一字段校验；昵称与文本业务协议
分别留在 `NicknameWire.h`、`SendTextWire.h`。HTTP 注册函数与原生业务函数分开，
让独立诊断 DLL 运行生产代码，而不是另外编写一份近似实现。

共享的是已核验的 ABI 和生命周期机制；每个新接口仍需独立确认构造入口、对象大小、
提交函数、线程上下文、请求字段及业务成功条件。完成回调、传输成功和业务完成
是不同阶段，应由业务层检查响应并建立请求关联。昵称仍保留 `self_nickname::Native`
及原参数接口；提取共享 protobuf 工具后已运行昵称协议回归测试。

### 原生入口、线程与 ABI 的定位

- 协议名称、URI、操作码和生成的 protobuf 构造/序列化函数可用于追踪调用链。
  `micromsg.ModUserInfo` 这样的名称只是线索，不能单独证明可调用入口。
- 分别确认请求构造、scene 提交、网络调度、回调调用和响应取得方式，
  不把同步包装函数与底层异步提交函数当作可互换入口。
- 昵称实例使用 CGI 服务的锁保护注册和原生网络任务队列提交。
  GetProfile 提交 RVA 为 `0x485240`，Oplog 提交 RVA 为 `0x19A1010`；
  这些入口及下述布局只适用于已验证构建，不能直接套用到其它接口或版本。
- 调查中，从普通远程线程调用同步 coroutine bridge `0x45FB50` 曾导致微信主进程
  异常退出，当时尚未提交昵称修改；恢复后改用异步 scene 路径并通过实测。
  该失败说明仅确认参数和地址还不够，必须验证入口所需的线程或 coroutine 上下文。

昵称实例的提交 ABI 为
`submit(service, request, nativeWxidString, options, functionStorage)`。
原生 scene 复制请求，接管回调 implementation；本例函数存储为 64 字节，
implementation 位于 `+0x38`。回调 ABI 为
`bool(implementation, scene**, errorType*, errorCode*)`，通过 scene 虚表 `+0x20`
取得响应。完整指针链、RVA 和对象布局见[昵称实现记录](edit-self-nickname-status.md)。

### 参数、协议与内存所有权

- 将 HTTP 层、协议层和原生调用层分开，让诊断工具复用生产函数及注册函数。
  昵称接口支持 `nickname` 和 `nick_name`，前者存在时优先；校验 JSON 对象、
  字符串类型、非空、UTF-8、NUL 和长度。4096 字节是防御性上限，不是微信业务限制。
- 修改完整资料中的单个字段时，先读取服务端现状，核对账号身份，并保留其它字段及
  未知 protobuf 字段，避免使用不完整资料或默认值覆盖账号设置。
- 区分传输错误、protobuf 解析错误和业务拒绝。昵称实例同时检查原生 errorType/errorCode、
  Oplog 返回值、结果数量以及单项结果，不能仅凭回调发生就返回成功。
- 微信使用的 MSVC string 和 std::function ABI 必须单独适配，不能直接传递本 DLL
  标准库对象。原生 protobuf 对象由原生分配器创建并按原生虚表销毁；回调对象由
  本 DLL 创建和销毁。借用的输入缓冲区、复制后的请求及 scene 接管的对象要分别记录。
- 本例原生 scene 会补充 BaseRequest，因此提交前使用已验证的 protobuf 部分解析入口。
  不应在其它协议中未经验证就跳过必填字段校验。

### 完成回调、超时与成功语义

昵称接口返回 `ret`、`retmsg`、`submitted`、`verified`：

| 情况 | 返回语义 |
|---|---|
| 提交前参数、版本、登录或身份检查失败 | 失败，submitted=false、verified=false |
| 服务端原昵称已匹配 | 成功，submitted=false、verified=true |
| 开始调用原生修改提交入口 | submitted=true，仅表示已进入提交阶段 |
| 业务成功且回读昵称匹配 | 成功，submitted=true、verified=true |
| 提交后超时、业务拒绝或回读失败/不匹配 | 失败，verified=false；保留已提交状态及具体原因 |

提交后失败不能一概解释为“服务端没有修改”。超时表示结果未知，回读失败也不能
撤销已经成功的业务操作；本例不自动重试。每次请求最多等待 30 秒，完整修改最多
包含三次请求。生产实现以互斥锁限制本 DLL 内同时执行的昵称修改流程；这不等于
锁住微信账号生命周期，也不保证超时后尚未完成的原生任务已经取消。

等待超时不会结束原生回调的所有权。共享完成状态需存活到原生持有者释放，
回调执行期间复制响应，不能让 HTTP 栈上的对象或失效的 scene 指针被晚到回调使用。
本例将承载回调代码的 DLL pin 到进程结束，防止代码卸载后仍被调用；这是当前实现
的生命周期选择，并非所有接口都必须采用的方式。

## 验证与交付记录

每个接口至少保存以下证据：目标模块构建、入口或指针链、字段/参数布局、
定位依据、生产代码位置、测试方式、实际结果和未验证范围。
不把账号值、手机号、令牌或全量进程内存写进参考文档；测试日志可只输出长度和匹配结果。

读取接口的测试应覆盖有效对象、短/长字符串、中文、空值、未登录、版本不匹配、
错误虚表、无效地址、异常长度和失败后输出清空。账号切换、退出和重新登录需要另做实测。

区分各个验证阶段：生产读取函数通过测试、接口源文件编译通过、完整 DLL 构建成功、
部署后实际加载新 DLL、HTTP 端到端验证通过。这些阶段不能相互替代。

GetSelfProfile 截至保存日期：独立生产读取函数的模拟测试和当前进程读取已通过，
GetSelfProfile.cpp 已编译为 Windows x64 对象；完整 MSVC DLL 尚未构建或部署，
新 HTTP 路由尚未做端到端验证，真实账号切换和重新登录尚未测试。

邮箱、QQ、省份、城市和签名尚未确认字段映射。当前响应保留兼容空值/0，
并通过 unavailable_fields 明确列出；这些字段不属于已完成的读取能力。

EditSelfNickname 截至保存日期的验证结果：

- 协议测试通过：中文、未知字段保留、账号不匹配、截断、重复字段和业务错误。
- 原生 protobuf 构造、部分解析、序列化和释放，以及异步资料查询完成回调均通过。
- 经用户授权，真实昵称已修改为 `test`；Oplog 业务返回成功，随后服务端查询确认匹配。
- 生产 HTTP 注册函数在微信进程的独立诊断 DLL 内通过短时本机服务测试：
  两个参数名均确认服务端昵称；错误 JSON、数组、缺少字段、数字、空字符串和 NUL
  均返回失败。测试服务已停止。
- MsgExtended.cpp 的 Windows x64 对象编译和诊断 DLL 链接通过；完整项目 MSVC DLL
  尚未构建和替换部署。诊断服务测试不能替代完整项目部署验证。
- 客户端缓存和界面没有强制刷新；登出、重登录、其它构建及超时后的实际行为尚未实测。

后续操作接口可按“协议测试 → 原生 codec → 只读原生请求 → 经授权的真实操作
→ 业务返回与回读 → 生产 HTTP 路由 → 完整构建与部署”逐层验证。
每层记录实际结果，先缩小 ABI 和生命周期问题，再扩大调用范围。

SendTextMsg_NoSrc 截至保存日期的验证结果：

- 无网络协议测试通过：UTF-8 文本、空 MsgSource、大整数 ID、错误收件人和
  ClientMsgId、重复结果、BaseResponse/单项业务拒绝、截断输入。
- 微信进程内原生 codec 通过：解析成功、必填字段完整、序列化完全匹配及对象释放。
- 复用生产注册函数的短时本机 HTTP 服务中，错误 JSON、数组、缺字段、错误字段类型、
  空文本及 NUL 均返回失败；未登录及启动器进程中的有效请求被账号检查拦截，
  `submitted=false`，没有产生消息发送。
- 登录后重新定位承载账号的进程，经用户授权向 filehelper 发送指定中文文本一次，
  返回 `ret=0, submitted=true, verified=true`，服务端消息 ID 为
  `1328595156538976503`，收件人和 ClientMsgId 均匹配。短时测试服务已停止。
- SendTextMsg.cpp Windows x64 对象编译、诊断 DLL 链接及共享组件提取后的昵称协议
  回归测试通过；完整项目 MSVC DLL 尚未构建和替换部署。
- 其它构建、群聊、登出竞争及超时行为未实测，客户端消息记录或界面同步未作为
  成功依据。详细记录见[文本发送实现与验证](send-text-nosrc-4.1.15.13.md)。

进程定位不能只保存上次 PID 或只检查 Weixin.dll 是否加载：实测中登录后另起了
承载账号的进程，启动器也可能具有相同模块和 CGI 服务。测试前应再次沿账号指针链
核验对象身份与登录状态，避免在错误进程中运行诊断。

真实修改、发消息等具有外部影响的诊断必须在已授权范围内执行；例如昵称 runner 的
`--edit-test` 会真实修改昵称，不能作为默认的无副作用测试。

## 后续接口记录模板

```text
接口名称及真实目标：
目标模块构建：
入口 RVA / 指针链：
对象身份和状态校验：
字段或参数布局及各自依据：
线程要求和内存所有权（操作接口）：
请求复制/借用、回调接管与释放时机：
异步完成、超时及模块卸载处理：
生产实现文件：
真实成功/失败的判断方法：
提交状态、业务确认与服务端回读的区别：
请求与响应关联字段、消息 ID 精度及重复请求语义：
模拟测试与进程实测结果：
完整构建、实际加载和 HTTP 验证结果：
未实现字段、未验证情形：
```

## GetFriendTimeline 原生网络实现（2026-10-08）

`POST /GetFriendTimeline` 和兼容路由 `POST /mmsnsuserpage` 已从缓存查询改为
原生 `mmsnsuserpage` CGI。它们直接返回服务端个人相册响应，不依赖 sns.db 落盘；
其它 Timeline 首页/下一页路由仍读取本地缓存。网络失败不回退旧缓存，不自动重试，
也不主动写数据库或强制刷新微信界面。详细证据见
[GetFriendTimeline 实现记录](get-friend-timeline-status.md)。

### 分层结构与执行流程

生产实现分别位于 `FriendTimelineWire.h`（协议）、`FriendTimelineNative.h`
（原生调用）和 `FriendTimelineRoute.h`（HTTP）；由 `src/Timeline.cpp` 注册路由。
独立诊断 DLL 和测试调用这些生产函数，避免另写一份近似实现。

```text
HTTP 对象、用户、count 和十进制 uint64 游标校验
  → 检查已核验的模块构建、当前账号身份及登录状态
  → 构造空缓存摘要、目标用户名和 MaxId
  → 原生异步 scene 复制请求、接管回调并提交到网络队列
  → 等待真实完成回调（最多 30 秒），复制并序列化响应
  → 校验传输、业务状态、对象数量、用户归属和游标范围
  → 再次核对当前账号，验证响应字符串后发布结果
  → 按 ID 选取本页，再按时间展示，保留 ID 分页游标
```

用户按 `user_name`、`username`、`wxid` 顺序选择首个非空字符串，必须指定。
`id` 首页省略、空字符串或 `"0"`；下一页使用响应 `next_id`。网络游标是朋友圈
uint64 ID 的十进制字符串，不是本地数据库 tid。`count` 默认 20，非正数用 20，
最多 100；它只限制返回数量，服务端决定单次网络页大小，不保证拉满。
错误 JSON、非对象、错误字段类型、NUL、非法 UTF-8、超长字符串和游标溢出均在提交前拒绝。
4096 字节输入上限及共享响应字符串 1 MiB 上限是本地防御性限制，不是微信业务限制。

### 构建专属入口与 ABI

以下只适用于 Windows x64 Weixin 4.1.15.13 已核验构建：
PE 时间戳 `0x6AB0A378`，SizeOfImage `0xC0AE000`。

| 项目 | 已核验入口或布局 |
|---|---|
| SnsUserPageRequest 构造、大小、虚表 | `0x5522F50`、`0x58`、`0x9813AB8` |
| 请求解析 / 序列化 | `0x554C7E0` / `0x554CED0` |
| 异步提交、CGI、URI | `0x3018880`、212、`/cgi-bin/micromsg-bin/mmsnsuserpage` |
| scene 构造、大小、虚表 | `0x3018BD0`、`0x1C0`、`0x93FE038` |
| scene 请求 / 响应成员 | `+0xA8` / `+0x100` |
| 响应 getter（虚表 +0x20） | `0x105BE10`，返回 `this+0x100` |
| scene BaseRequest 准备 | `0x3018EE0` |
| SnsUserPageResponse 虚表 / 序列化 | `0x9813BD8` / `0x554F510` |
| SnsObject 虚表 / 序列化 | `0x9812A28` / `0x5533580` |
| 客户端相册请求构造 | `0x2FE4950` |

通过 URI、协议名称引用、protobuf 虚表、构造、解析和序列化函数交叉定位。
异步提交 ABI 是 `(service, request, nativeString, options, functionStorage)`；
提交函数复制请求、转移回调 implementation，经 service 虚表 `+0x28` 注册 scene，
再调用 `0x67C180` / `0x67C520` 调度到原生队列。
独立确认回调转移方式和响应 getter 后，才复用 `NativeCgi` 的完成状态、内存所有权、
超时等待和 DLL pin。同步包装 `0x3015C40` 需要 coroutine 上下文，不用于此路径。
超时不取消原生回调所有权，也不能报告网络查询成功。

### 请求字段与映射依据

客户端 `0x2FE4950` 在请求 `+0x10` 写空摘要，在 `+0x18` 复制用户名；
结合序列化函数确认字段号。字段类型相同不能证明语义相同。

| 请求字段 | 内容 |
|---|---|
| 1 | BaseRequest，由原生 scene 补充 |
| 2 | FirstPageMd5 空字符串，要求完整响应 |
| 3 | 目标用户名 |
| 4 | MaxId：首页 0，下一页为游标 |
| 5、6、7、9 | 对应客户端初始相册请求的零值，不开放未核验的筛选参数 |

原生 codec 验证中，部分请求可解析且序列化回读完全一致；提交前缺少 BaseRequest，
`IsInitialized=false`，该字段由 scene 在网络序列化前补充。
初版误将用户名写到字段 2，真实服务端返回 `Ret=-2`；对照客户端构造后修正为字段 3。
这一记录说明 codec 回读一致只能确认编码可解析，不能单独确认业务字段映射正确。

### 响应校验与业务码 207

响应字段 1 为 BaseResponse，字段 3 为数量，字段 4 为重复 SnsObject；
字段 12 为客户端使用的显示提示，原生对象成员为 `+0x60`。
SnsObject 字段 1/2/3/4/5 分别为 ID、用户名、昵称、时间和 ObjectDesc。
ObjectDesc 是 SKBuiltinBuffer_t，字段 1 长度必须与字段 2 XML 字节数一致。

要求数量与实际对象数一致，核心字段唯一且未截断，每条对象属于请求用户，
ID 非零且不重复，下一页 ID 小于 MaxId；XML、昵称和提示需为有效 UTF-8 且无 NUL。
显示提示另有 4096 字节上限。失败不发布部分对象。

业务状态必须结合客户端处理分支和真实响应确认，不能简单将所有非零码视为同一种错误：

| 状态 | 本接口语义 |
|---|---|
| BaseResponse.Ret=0 且全部校验通过 | 普通成功，`verified=true`、`visibility_limited=false` |
| BaseResponse.Ret=207 且全部校验通过 | 可见时间范围受限的有效响应，`verified=true`、`visibility_limited=true` |
| 其它未核验非零业务码 | 失败，保留真实业务码及可用服务端提示，`verified=false` |
| 传输、结构、对象关联或账号一致性失败 | 失败，不返回部分数据，`verified=false` |

客户端 `0x2FE6A41` 对 207 单独分类，在 `0x2FE6A51` 设置相册状态 1 并读取
响应 `+0x60` 的文本。本次指定目标的真实响应为业务码 207、对象数量 0，提示
**“朋友仅展示最近三天的朋友圈”**。返回空数组只描述此次可见查询结果，不能解释为
对方从未发布动态，也不代表可以获取可见范围以外的记录。
207 同样要通过数量、对象及账号校验；不能将其它非零码一并改为成功。

HTTP 对有效响应返回 `ret=0`、`submitted=true`、`verified=true`、`source=network`、
`business_ret`、`visibility_limited`、`count`、`server_count`、`user_name` 和 `data`。
普通成功 `retmsg=success`；207 使用服务端原文作为 `retmsg`，并保留 `server_message`。
失败返回 `ret=-1`、具体错误、空 data 和真实提交/验证状态。
`verified` 表示已取得并校验服务端相册响应，不表示客户端缓存或界面已同步。

每条记录保留字符串 `sns_id`、`user_name`、字符串 `create_time`、`nickname` 和
原始 XML `content`；可提取时补充 `content_desc`，去除完整 CDATA 包装。
不编造本地数据库 tid；标签提取不是完整 XML 解析器。
先按 ID 降序选取 count 条，再按时间稳定降序展示，`next_id` 为选中记录的最小 ID。
调用方必须使用该游标，不能取展示数组末条 ID，否则截断或排序可能导致漏记录。
此逻辑不保证跨页整体时间降序。

### 验证结果与交付边界

- Windows x64 Zig 0.16.0 编译并运行参数、协议和路由测试通过；
  Timeline.cpp 对象编译、诊断 DLL 链接、SQLite 查询回归和差异检查通过。
- 微信进程内原生请求解析、序列化回读和释放通过；修正字段后，当前账号相册网络请求
  返回业务码 0、10 条对象、`submitted=true`、`verified=true`。
- 复用生产注册函数的短时本机 HTTP 服务中，指定好友首页返回 3 条，下一页返回 3 条，
  各页时间降序、两页无重复；兼容路由返回 1 条。错误参数均在提交前失败。
- 207 修正新增空页、可见对象、数量错误、重复提示、未核验 208、业务码溢出和路由状态测试，
  均通过。给定目标的生产 Native Fetch 及两个 HTTP 路由实测返回
  `ret=0, business_ret=207, verified=true, visibility_limited=true, count=0`。
- 207 修正后的额外普通好友 HTTP 回归在短时服务停止后运行，连接被拒绝，该次运行未完成；
  不将此前的普通分页实测冒充为此次修正后的完整 HTTP 回归。短时诊断服务已停止。
- 完整 MSVC 项目 DLL 尚未构建和替换部署，运行服务需重建并加载更新后生效。
  其它构建、真实账号切换、超时后的晚到回调、其它业务码和所有筛选模式未实测。

可复用的是分层实现、独立定位、异步生命周期和业务状态核验方法；
上述 RVA、对象布局、207 语义和零值参数不能未经验证直接套用到其它接口或构建。

## sns_like 点赞接口（2026-10-09）

`POST /sns_like` 已接入原生异步 `mmsnscomment` CGI，向指定朋友圈添加点赞。
生产路径由 `include/SnsLikeWire.h`（协议）、`include/SnsLikeNative.h`（原生调用）、
`include/SnsLikeRoute.h`（参数与响应）组成，在 `src/Timeline.cpp` 的 `Route_Timeline` 注册。
完整定位和实测记录见 [sns_like 原生实现记录](sns-like-status.md)。

### 参数与执行流程

```json
{
  "sns_id": "15027695064333161080",
  "to_wxid": "wxid_0xexx8mmtng912"
}
```

`sns_id` 必须是非零 uint64 十进制字符串，不能用 JSON 数字代替；字符串形式避免
调用方丢失大整数精度。`to_wxid` 为朋友圈作者，必须非空、无 NUL、有效 UTF-8，
且不超过 4096 字节。该上限是本地防御性限制，不是服务端业务限制。
JSON 非对象、缺字段、错误类型、非法 ID 或溢出均在提交前拒绝。

```text
HTTP 参数校验
  → 互斥锁限制本 DLL 内并发点赞流程
  → 核验模块构建、登录状态并读取当前账号
  → 构造 SnsActionGroup / SnsAction 点赞请求
  → 原生 scene 复制请求、接管回调并提交网络队列
  → 等待完成回调（最多 30 秒），复制并序列化响应
  → 检查传输结果、BaseResponse.Ret 和响应朋友圈 ID
  → 再次核对当前账号，业务确认后返回 verified=true
```

接口不写本地数据库，也不强制刷新客户端界面。超时表示结果未知，不自动重试；
互斥锁不能取消超时后的原生任务，也不保护微信账号的完整生命周期。

### 已核验构建、入口与 ABI

以下仅适用于 Windows x64 Weixin 4.1.15.13：PE 时间戳 `0x6AB0A378`，
SizeOfImage `0xC0AE000`。每次调用复用 `ProfileMemory` 的构建和登录校验。

| 项目 | RVA / 布局 |
|---|---|
| SnsCommentRequest 构造、大小、虚表 | `0x5523190`、`0x28`、`0x9813FA8` |
| 异步提交、CGI、URI | `0x301E860`、213、`/cgi-bin/micromsg-bin/mmsnscomment` |
| scene 构造、大小、虚表 | `0x301EBB0`、`0xF0`、`0x93FEC78` |
| scene 请求 / 响应成员 | `+0xA8` / `+0xD0` |
| 响应 getter（虚表 +0x20） | `0x789780`，返回 `this+0xD0` |
| SnsAction 构造 / 序列化 | `0x55230C0` / `0x55539A0` |
| SnsActionGroup 序列化 | `0x5554F90` |
| SnsCommentRequest 序列化 | `0x5555B80` |
| 客户端动作构造依据 | `0x2FE91E0` |

通过 URI 引用 `0x301E8DF`、protobuf 虚表、构造与序列化函数，以及客户端动作
构造交叉核对。异步提交 ABI 为 `(service, request, nativeString, options, functionStorage)`；
提交函数复制请求，转移 MSVC std::function 的 `+0x38` implementation，
经 CGI 服务虚表 `+0x28` 注册 scene，再调用 `0x67C180` / `0x67C520` 调度到原生队列。
独立确认响应 getter 与回调所有权后，复用 `NativeCgi` 的完成状态、30 秒等待、DLL pin
和晚到回调生命周期。不能仅凭昵称、文本或相册接口的 ABI 相似就套用其入口。

### 点赞请求协议与成功语义

SnsCommentRequest 外层字段 2 为 SnsActionGroup，字段 1 BaseRequest 由原生 scene 补充。
SnsActionGroup 字段 1 为朋友圈 ID，字段 3 为 CurrentAction；当前动作编码如下：

| SnsAction 字段 | 当前发送内容 |
|---|---|
| 1 | 当前登录账号 wxid |
| 2 | 朋友圈作者 `to_wxid` |
| 5 | Type=1，添加点赞 |
| 6 | Source=0 |
| 7 | 当前 Unix 秒时间 |

客户端 Type=5 对应移除动作，当前接口只开放添加点赞。客户端 Source 来自业务对象
`+0x244`；本实现发送 0，已通过指定动态的一次真实业务验证，不能宣称适用于所有场景。
时间只用于请求，不作为成功证据。

响应字段 1 为 BaseResponse，字段 2 为 SnsObject。仅原生传输成功、
BaseResponse.Ret=0、SnsObject 字段 1 的 ID 与请求相同，且回调后账号一致时成功。
缺失、重复、截断、业务拒绝或 ID 不匹配均返回失败。

HTTP 返回 `ret`、`retmsg`、`submitted`、`verified`；进入业务解码后保留真实
`business_ret`，未取得业务结果时为 -1。`submitted=true` 表示已进入原生提交阶段；
`ret=0, verified=true` 表示服务端确认指定朋友圈操作成功，不表示客户端界面已同步。
提交后失败或超时不能解释为服务端没有产生效果。

### 验证结果与交付边界（2026-10-09）

- Zig 0.16.0 Windows x64 协议与路由测试通过：大整数 ID、动作类型、业务拒绝、
  ID 不匹配、缺失/重复对象、截断、错误参数、溢出、NUL、非法 UTF-8、长度限制和超时状态。
- `Timeline.cpp` Windows x64 对象编译、原生诊断 DLL 链接及差异检查通过。
- 微信登录后重新定位主进程，生产 ProfileMemory 构建与登录校验通过；原生 codec
  返回 `parsed=true, roundtrip_matches=true`，原生请求对象正常释放。
- 经用户明确授权，诊断 DLL 调用生产 `SnsLikeRoute::Handle` 和 `SnsLikeNative::Like`，
  对指定动态提交一次真实点赞，返回 `ret=0, business_ret=0, submitted=true, verified=true`；
  响应 SNS ID 匹配，完成后账号一致性检查通过。没有重复提交或取消点赞。
- 完整 MSVC 项目 DLL 尚未构建和替换部署，完整项目 HTTP 端到端验证未完成。
  进程内生产 Handle 测试不能替代部署后的 HTTP 验证。
- 其它构建、其它动态的 Source=0 兼容性、真实账号切换、超时晚到回调和界面同步未实测。

测试工具为 `tools/sns_like_test.cpp`、`tools/sns_like_native_test.cpp` 和
`tools/sns_like_native_runner.py`。runner 必须显式选择 `--codec` 或 `--like`；
`--codec` 不发送网络操作，`--like` 会对诊断源码中的指定动态真实点赞一次。
每次进程内测试前重新核验登录进程，不能复用旧 PID 或只检查模块是否加载。

## IsLoginStatus（2026-10-15，4.1.15.55）

`POST /IsLoginStatus` 无参数，已从数据库句柄与全局缓存判断改为独立原生读取。
原实现以 `contact.db` 句柄是否存在作为登录依据，并返回 `SelfInfo` 缓存，
不能保证退出、初始化或账号切换时的状态和资料属于当前账号。

### 代码结构与复用范围

- `include/LoginStatusMemory.h`：独立校验新版构建、账号对象和登录位，安全读取账号资料。
- `include/LoginStatusRoute.h`：`Handle` 转换响应，`Register` 注册 HTTP POST。
- `src/ContactsMgr.cpp`：在 `Route_ContactsMgr` 中调用 `login_status::Register`。
- `tools/login_status_test.cpp`：直接复用生产 `Read`、`Handle`、`Register`，执行模拟读取和本机 HTTP 测试。
- [IsLoginStatus 详细定位与验证记录](is-login-status-4.1.15.55.md)：完整指令依据及诊断运行方法。

仅复用 `ProfileMemory` 的 `ReadValue`、`ReadString` 等安全读取工具，不调用其旧版
`Read`，不修改其它接口的构建门槛。本次只核验 `IsLoginStatus`，不能据此宣称
`GetSelfProfile`、原生 CGI 或其它接口已适配 4.1.15.55。

### 新版构建与原生定位

已核验实际加载的 Windows x64 `Weixin.dll` 4.1.15.55：
PE 时间戳 `0x6ACCBF76`，SizeOfImage `0xC34F000`。
以下均为该构建的模块相对地址或对象内偏移，不能沿用旧版数值。

| 项目 | 新版位置 | 定位依据 |
|---|---|---|
| manager 全局入口 | RVA `0xBB92FC0` | 原生 getter `0x45D10` 从该全局取值 |
| account 成员 | manager `+0x68` | manager 虚表 `+0x60` 的 getter `0x469A0` 返回该 shared_ptr；`+0x70` 为控制块 |
| account 虚表 | RVA `0x8FD1498` | 构造函数 `0x37B880` 在 `0x37B8B5` 加载并写入 |
| 登录标志 | account `+0x38` 的 bit 0 | 虚表首项 `0x389A50` 执行 `movzx eax,byte ptr[rcx+0x38]; and al,1; ret` |
| wxid / alias | account `+0x48` / `+0x68` | 虚表 getter `0x3855C0` / `0x3855D0` 返回对应字段地址 |
| nickname | account `+0x88` | 账号对象中相邻 MSVC 字符串及既有布局核对；未做独立界面核对 |

调查时通过模块 `.data` 中的候选指针、账号虚表、原生 getter 和构造函数交叉核对。
生产路径只沿固定指针链读取，不扫描内存、不查询数据库、不读取账号目录、不访问 `SelfInfo`。

### 读取流程与一致性检查

```text
清空输出
  → 检查 PE 签名、AMD64 架构、时间戳和映像大小
  → 读取 manager → account，校验账号虚表
  → 读取 account+0x38 的 bit 0
  → 已登录时读取 wxid、alias、nickname；未登录时不读取残留资料
  → 再次核对 manager、account、虚表、登录位；已登录时复读并比较三个字段
  → 全部通过后发布本次结果；任一步失败返回具体错误
```

资料读取检查 MSVC 字符串的长度、容量、内联/堆存储、UTF-8 和 NUL，
采用 `ReadProcessMemory` 并核对实际读取字节数，失败不发布半份资料或上次账号数据。
空 manager/account、虚表不匹配或不可读仅证明状态不可用，不能直接解释成未登录。
双次读取是尽力而为的一致性检查，不持有微信内部锁，无法排除 ABA 切换和完整生命周期竞争。

### 返回语义

| 读取结果 | 返回字段 | 账号资料 |
|---|---|---|
| 确认已登录 | `ret=0, retmsg=success, status=1, is_login=true` | 本次原生读取的 `wxid`、`alias`、`nickname` |
| 确认未登录 | `ret=0, retmsg=success, status=0, is_login=false` | 不返回 |
| 构建不支持、对象不可用、字段无效或快照变化 | `ret=-1, status=-1, is_login=null`，`retmsg` 说明原因 | 不返回 |

调用方必须先检查 `ret`；`is_login=null` 表示未知，不能转成已确认未登录。
保留原有 `status`、`is_login` 字段，新增明确的读取失败语义；Apifox 对应接口说明已同步。
当前实现和本次验证均为 HTTP POST，未新增或验证 WebSocket 分发。

### 验证结果与交付边界

- Zig 0.16.0 Windows x64 测试程序及 `ContactsMgr.cpp` 对象编译通过，仅有既有第三方字面量运算符警告。
- 模拟映像覆盖登录、退出、其它标志位、错误构建/虚表、非法 UTF-8、超长或不可读字符串、
  空 wxid、空/不可读 manager、空 account 和失败输出清空；真实本机 HTTP 验证登录、退出及未知状态响应通过。
- 重新枚举实际微信 PID 和模块基址后，生产读取及复用生产注册函数的短时本机 HTTP POST
  验证通过，`logged_in=1`。诊断只读目标进程，不注入 DLL、不修改微信内存、不提交微信网络业务请求。
- Apifox JSON、项目 XML 解析和差异检查通过。
- 完整项目 `core.dll` 尚未重建和替换部署；诊断 HTTP 验证不等同于完整 Hook DLL 部署后的端到端验证。
- 真实退出/重新登录、账号切换、其它构建及高并发生命周期变化尚未实测。

## GetSelfProfile（2026-10-15，4.1.15.55）：实现规则与方式

对应接口集合为本项目 `PCWeixinHook_ShowDoc_All_Apifox_Merged.apifox.json` 中的
“个人信息 / 获取个人信息”：`POST /GetSelfProfile`，无参数，请求体可为 `{}`。
详细证据与限制见 [GetSelfProfile 4.1.15.55](self-profile-4.1.15.55.md)。

### 实现规则

1. **先核对接口合约，再适配实际构建。** 从接口集合确认路由、方法和参数；
   从实际加载模块确认版本、AMD64、PE 时间戳和映像大小。目录名、旧版 README、
   旧 DLL 返回结果均不能证明当前代码适配成功。
2. **隔离版本专属读取入口。** 新版使用 `include/SelfProfileMemory.h` 中的
   `self_profile_memory::Read`；旧版 `profile_memory::Read` 继续保留原构建门槛。
   只复用 `ReadValue`、`ReadString` 等安全工具，不扩大旧版原生 CGI 或其它操作接口的支持范围。
   共用指针链也不意味着其它字段、函数入口、ABI 已完成核验。
3. **分别证明对象身份与字段语义。** 用指针链、虚表、原生 getter 和构造函数核对账号对象；
   对每个字段单独记录依据。相邻位置、字符串长度及旧版布局只能提供线索，不能单独证明语义。
   本次 phone 为 account+0xA8 的 11 字节字符串候选，尚未与账号设置独立核对；
   nickname 也未完成独立界面核对，不得将这些限制改写为全部字段已验证。
4. **成功只发布本次完整读取结果。** 入口清空输出，检查构建、账号虚表和登录位；
   校验字符串元数据、实际读取字节数、UTF-8 及 NUL；复核账号链、虚表、登录位与全部字段后
   才发布结果。失败返回具体原因，不返回半份资料或上次账号缓存。
5. **明确未实现字段。** email、qq、proiv、area、signinfo 尚未定位，兼容占位分别为
   空字符串或 0，并通过 `unavailable_fields` 标明；占位不能解释为真实账号值。
   不能为了填满响应而回显参数、查询无关缓存或猜测字段。
6. **按验证层次记录结论。** 区分静态定位、模拟测试、对象编译、完整 DLL 构建、部署和
   运行实测。临时诊断 HTTP 服务复用生产路由，仍不能代替完整 Hook DLL 部署后的验证。
   使用普通本机分析工具时如实注明工具来源，不将其结果称为 REA Evidence。

### 代码组织与读取方式

| 文件 | 职责 |
|---|---|
| [SelfProfileMemory.h](../include/SelfProfileMemory.h) | 新版构建校验、原生账号读取、字符串验证及前后快照比较 |
| [SelfProfileRoute.h](../include/SelfProfileRoute.h) | `Handle` 生成响应，`Register` 注册 POST；允许注入读取函数供测试使用 |
| [GetSelfProfile.cpp](../src/GetSelfProfile.cpp) | 调用注册函数；`GetSelfProfile` 将同一原生读取结果转为 `SelfInfo_t` |
| [self_profile_test.cpp](../tools/self_profile_test.cpp) | 直接复用生产 `Read` 和 `Register`，运行模拟及只读实时 HTTP 检查 |

```text
POST /GetSelfProfile
  → self_profile_memory::Register → Handle
  → Read：清空输出并校验实际模块构建
  → manager = *(module + 0xBB92FC0)
  → account = *(manager + 0x68)
  → 校验虚表 module + 0x8FD1498，确认 account+0x38 的 bit 0
  → 安全读取 wxid(+0x48)、alias(+0x68)、nickname(+0x88)、phone 候选(+0xA8)
  → 再次校验 manager/account/虚表/登录位及四个字段
  → 全部一致后发布数据，由 HTTP 层序列化
```

上述数值只适用于本次核验的 AMD64 构建：时间戳 `0x6ACCBF76`、
SizeOfImage `0xC34F000`。生产读取不扫描内存、不查询数据库、不读取账号目录或全局 `SelfInfo`。
双次读取不持有微信内部锁，无法排除 ABA 变化与完整对象生命周期竞争。

成功返回 `ret=0, retmsg=success` 和资料及 `unavailable_fields`；
未登录、构建不匹配、对象不可用、字段无效或快照变化返回 `ret=-1` 与具体 `retmsg`，
不返回账号字段。与登录状态接口不同，个人资料接口在确认未登录时不能返回资料读取成功。
本次仅注册 HTTP POST，未新增或验证 WebSocket 分发。

### 可复用的验证步骤与本次边界

1. 模拟映像调用生产读取函数：覆盖内联/堆字符串、中文 UTF-8、未登录、错误构建/虚表、
   非法或超长字符串、不可读内存、空 wxid/账号及失败输出清空。
2. 临时本机 HTTP 服务调用生产注册函数：验证成功响应与未登录、错误构建的失败响应，
   确认失败不泄露残留资料。
3. 重新枚举微信 PID、实际模块路径和基址，以只读进程权限运行同一生产读取及 HTTP 路由；
   日志仅记录字段长度，避免保存个人值。不能复用历史 PID、基址或旧测试程序的结果。
4. 编译测试程序及修改的生产翻译单元，检查接口集合 JSON 与差异；
   若要宣称部署可用，还必须另行完成完整 DLL 构建、加载确认和部署后接口验证。

本次 Windows x64 Zig 0.16.0 测试程序、模拟读取、临时 HTTP 成功/失败测试及
当前微信进程的生产读取/临时 HTTP 检查通过；四字段字节长度为 19/14/6/11。
`GetSelfProfile.cpp` 使用 `-fms-extensions` 编译为对象通过（既有二维码代码使用 SEH），
仅有既有第三方字面量运算符警告；Apifox JSON 解析与差异检查通过。
完整 Hook DLL 尚未构建和替换部署；真实退出重登、账号切换、高并发竞争及其它构建未实测。

## 个人信息九接口逐项适配（2026-10-15，当前项目）

接口清单以 PCWeixinHook_ShowDoc_All_Apifox_Merged.apifox.json 的“个人信息”为准。
本轮按顺序核验；旧版结果不能代替 4.1.15.55 验证。

### 1. GetSelfProfile

复用 SelfProfileMemory.h / SelfProfileRoute.h 的现有新版原生读取。
本轮重新编译 tools/self_profile_test.cpp 到 .review/self_profile_batch_test.exe，
模拟读取及临时生产 HTTP 路由通过；重新枚举微信进程和 Weixin.dll 后，
只读原生读取与临时 HTTP 检查通过，四字段字节长度为 19/14/6/11。
完整 DLL 尚未构建部署；phone 语义仍待设置界面独立核对，五个 unavailable_fields 未定位。
原理及边界见 self-profile-4.1.15.55.md，不将已有实现改写为九接口全部完成。

### 2. getonlineinfo

新增 include/OnlineInfoRoute.h，由 src/GetSelfProfile.cpp 注册。
复用独立核验的 login_status::Read，直接读取新版账号登录位并检查构建、虚表和前后快照。
移除“个人资料读取失败即未登录”的错误推断。
成功返回 ret=0、online_status=1/0、is_online=true/false；读取失败返回 ret=-1、
online_status=-1、is_online=null 及具体错误。scope=local_client_login 明确本机登录态范围；
未实现服务端多设备在线列表或心跳可达性。
测试使用 tools/online_info_test.cpp，复用生产注册与读取函数，覆盖模拟及只读实时 HTTP。

本轮 getonlineinfo：Zig x64 编译、模拟账号读取得到登录/退出/错误构建、真实临时 HTTP 三态测试通过；
重新枚举当前微信进程后，同一生产读取及临时 HTTP 验证通过，logged_in=1。

### 3. Busif2fgetqrcodev2（未完成）

src/MsgExtended.cpp 移除固定 payapp 通用 URL。原生请求入口、协议和业务结果尚未核验，
现返回明确失败。失败保护不算实现。

### 4. GetSelfAddress（未完成）

移除昵称/手机号/地区拼接收货地址的路径。账号资料不等于收货地址列表；
尚缺地址服务的请求构造、提交和响应映射，现返回明确失败。

### 5. GetQRCode（已实现新版原生路径）

SelfQrRoute.h → SelfQrNative.h → SelfQrCgi.h → 原生 CGI 168。
先前的全内存扫描、缓存和 URL 回显已移除；当前查询当前账号二维码，不重置。
本轮确认请求构造、异步场景、服务端返回 field 9 URL，并通过 native codec、
真实原生查询及微信进程内短时生产 HTTP 验证。完整 Hook DLL 未构建部署。
具体入口、PB 字段、生命周期与验证边界见文末专项实现补充和
[GetQRCode 实现记录](self-qrcode-status-4.1.15.55.md)。

### 6. EditSelfNickname（已适配 4.1.15.55）

新版 GetProfile/Oplog 构造、异步入口及 ABI 已分别核验；保留未知资料字段，提交后从服务端回读。
本轮 native codec、协议/HTTP 和真实改为 test 后恢复原昵称均通过，完整 Hook DLL 未部署。
入口及验证边界见 [新版昵称实现记录](edit-self-nickname-4.1.15.55.md) 和文末补充。

### 7. mod_self_nick_signature（已适配 4.1.15.55）

原生 GetProfile 核对账号后，保留 ModUserInfo 其它及未知字段，仅更新直接字符串 field 13，
经 Oplog CmdId=1 提交并服务端回读。允许空字符串清空签名，昵称与签名共用 DLL 内写入锁。
协议、HTTP、native codec、锁互通和真实改为 test 后恢复空签名的验证均通过；昵称未改变。
完整 Hook DLL 未部署。见 [签名实现与验证](edit-self-signature-4.1.15.55.md) 及文末补充。

### 8. upload_head_img（未完成）

新增明确失败路由，无文件上传。待核验原生上传任务、输入文件所有权、
异步完成和服务端头像回读；文件存在或提交成功不能代替业务完成。

### 9. get_config_path（未完成）

新增明确失败路由。尚未定位微信真实配置目录 getter；Hook 的 wx.ini 目录、
代理 DLL 目录和系统 Documents/AppData 不能直接替代该接口。

上述明确失败保护统一使用 include/PersonalInfoAvailability.h：ret=-1，
implemented/submitted/verified=false；不当作完成的原生接口。
本轮缺口及静态调查边界见 [九接口状态](personal-info-status-4.1.15.55.md)。
GetSelfProfile.cpp 的本轮生产对象编译通过；完整 DLL 尚未构建部署。

本轮最终检查：GetSelfProfile.cpp、MsgExtended.cpp 均由 Zig Windows x64 编译为新对象成功；
接口集合 JSON 解析及 git diff --check 通过。完整 DLL 未构建部署。

### Busif2fgetqrcodev2 专项定位补充（2026-10-15）

已读取原始 ShowDoc 合约：空对象请求，真实响应字段为 qrcode_url（wxp://f2f…）与脱敏 name。
新版静态调查定位 TenPayRequest/Response、异步提交候选 RVA 0x53C82B0 和场景构造 0x53C85F0。
该封装默认 CGI=385，允许覆盖 URI，但没有在已解码路径中提供 CGI 编号替换参数。
因此不能简单把 URI 换成收款码路径并宣称实现。未对这些候选入口做运行调用。
具体指令依据、搜索范围、复现工具及仍缺的协议证据见
[收款二维码专项定位记录](collection-qrcode-status-4.1.15.55.md)。
生产路由仍明确失败；本补充仅为定位记录，不计作完成的原生实现。

## GetQRCode 本人二维码原生实现（2026-10-15）

已接入 SelfQrRoute → SelfQrNative → SelfQrCgi；CGI 168，URI /cgi-bin/micromsg-bin/getqrcode。
新版异步提交 RVA 0x1726630，请求构造 0x382BA90（48 字节），scene 构造 0x1726970，
请求/响应偏移 +0xA8/+0xD8，service 虚表 0x8FCD388；仅用于本次已核验 4.1.15.55 构建。
旧版 NativeCgi 的偏移和支持范围没有扩大。

请求使用 SKBuiltinString_t 包装当前 wxid，opcode/style=0；scene 补充 BaseRequest。
Native codec 往返、真实异步请求及微信内短时生产 HTTP 路由均通过；服务端业务码为 0，
第 9 字段是 44 字节个人二维码链接，直接返回 QRCode_URL 和兼容 qr_url。
非法参数不提交，业务失败/账号变化/超时不发布链接；超时不自动重试。
回调复制响应，自有状态及模块 pin 保证晚到回调代码存活；不把提交成功当作业务成功。

详细入口、指令证据、PB、所有权、复现步骤与部署边界见
[GetQRCode 实现与验证](self-qrcode-status-4.1.15.55.md)。
完整 Hook DLL 尚未构建部署；无重置二维码、无发消息操作。


## EditSelfNickname 4.1.15.55 原生实现与实测（2026-10-15）

生产路径保留 EditSelfNicknameRoute → EditSelfNickname，改用新版账号构建检查和
SelfQrCgi 中此前已验证的异步 ABI；本轮独立核验以下 GetProfile/Oplog 入口后才复用。
旧版 NativeCgi 的偏移未更改，其它旧版业务不随本次扩展支持范围。

| 项目 | 当前核验的 RVA / 布局 |
|---|---|
| GetProfile 提交 / CGI / 构造 | 0x49A1C0 / 302 / 0x1009600（32 字节） |
| GetProfile scene / 虚表 / 请求响应 | 0x49A500 / 0x8FDC488 / +0xA8、+0xC8 |
| Oplog 提交 / CGI / 构造 | 0x19B7D40 / 681 / 0x3B1B040（24 字节） |
| Oplog scene / 虚表 / 请求响应 | 0x19B8080 / 0x92E4418 / +0xA8、+0xC0 |
| 原生服务 | manager=*(base+0xBB92FC0)，service=*(manager+0x2C8)，虚表 0x8FCD388 |
| 部分解析 / 分配 / sized free | 0x697160 / 0x754C41C / 0x754C460 |

只支持核验的 AMD64、timestamp=0x6ACCBF76、SizeOfImage=0xC34F000 构建。
请求由原生场景复制，回调通过 scene 虚表+0x20 获取响应，不硬编码两种响应偏移。
每次最多等待 30 秒，超时不自动重试；共享状态和模块 pin 支撑晚到回调。

先 GetProfile 核对服务端账号，保留完整 ModUserInfo 其它及未知字段，仅替换昵称 field 3，
经 Oplog CmdId=1 提交；业务确认后再次 GetProfile，比对昵称并复核本机账号后才成功。
同昵称直接返回 verified=true、submitted=false。生产 Edit 和 HTTP 均校验非法输入。
不写 account 或 SelfInfo，不用回显或提交阶段返回码替代业务结果。

本轮协议测试、生产 HTTP 参数测试、新版 native codec 往返及原生 GetProfile 通过。
短时生产 HTTP 真实测试两个参数名，同昵称不提交，非法输入均失败。
经本会话明确授权，将真实账号昵称改为 test 后恢复服务端原昵称，两次均
ret=0、submitted=true、verified=true；另一次独立 GetProfile 确认已恢复。
改名前保存 DPAPI 加密恢复备份，确认恢复后已删除；日志未保存明文昵称或账号值。
MsgExtended.cpp 新对象编译通过；完整 Hook DLL 尚未构建替换部署。

完整证据、复现工具、异步所有权和未实测边界见
[EditSelfNickname 4.1.15.55 实现与验证](edit-self-nickname-4.1.15.55.md)。


## 修改个人签名：4.1.15.55 原生实现（2026-10-15）

EditSelfSignatureRoute → EditSelfSignature → 已核验的新版 GetProfile/Oplog 异步服务。
接口 POST /api/mod_self_nick_signature，newSignature 字符串，空字符串清空。
SignatureWire 只替换或新增 ModUserInfo field 13（直接字符串），其余及未知字段保留原字节。
当前 DLL 的 ModUserInfo 序列化函数 0x101D370，在 0x101D6C1 读取对象+0x58，
0x101D6C5 指定 field 13，0x101D6CD 调用字符串序列化；公开协议定义及真实业务回读交叉核验。

复用本项目新版昵称接口已核验的 GetProfile 0x49A1C0/构造 0x1009600，
Oplog 0x19B7D40/构造 0x3B1B040；只适用 AMD64、timestamp=0x6ACCBF76、SizeOfImage=0xC34F000。
共享原生 ABI 不等于推断签名字段；本轮对 field 13 做了单独静态及业务验证。
回调、对象复制/释放和超时生命周期沿用新版 helper；旧版 NativeCgi 未扩大支持范围。

先查询完整本人资料并核对账号，再 Oplog CmdId=1，只在命令业务成功、签名回读匹配、
昵称未变和最终账号复核通过时返回成功。同签名不提交；超时不重试，不回显输入伪造结果。
SelfProfileMutation.h 提供昵称/签名共享写入锁，两个完整资料写操作在同一 DLL 内串行化。
跨 DLL、其它客户端及超时后仍在队列中的请求不受此锁协调。

本轮 wire/生产 HTTP 测试、新版 native codec、原生资料查询和生产 HTTP 同签名测试通过。
持有共享锁时，签名与昵称路由都拒绝提交。昵称路由回归测试通过。
按用户明确授权，真实改签名为 test 后恢复原来的空签名，两次 ret=0、submitted=true、verified=true；
独立 GetProfile 确认已恢复且昵称未变化。DPAPI 加密恢复备份确认后已删除。
完整证据、代码组织、HTTP 合约和工具见 [签名实现记录](edit-self-signature-4.1.15.55.md)。
完整 Hook DLL 尚未构建替换部署；不将诊断 DLL 结果等同完整代理 DLL 已部署。

签名本轮最终检查：GetSelfProfile.cpp、MsgExtended.cpp 重新编译为 Windows x64 对象均通过；
Apifox JSON 解析、诊断 runner Python 语法编译与 git diff --check 通过。
完整 Hook DLL 未构建替换部署。

## 修改头像：4.1.15.55 原生实现（2026-10-15）

生产 POST /api/upload_head_img（filepath）已接入原生 CGI157 /cgi-bin/micromsg-bin/uploadhdheadimg。
使用自有 scene 实例适配 Pack/Parse，原生 BaseRequest、服务和调度器负责账号会话及网络；
未修改全局虚表，没有通过 Oplog field8/9 替代上传，也没有使用固定成功或参数回显。
构建限制 AMD64、timestamp=0x6ACCBF76、SizeOfImage=0xC34F000。
scene 构造0x49A500、原虚表0x8FDC488、BaseRequest准备0x49A810，
IOBuffer追加/数据/长度0x6979C0/0x697A00/0x697A30，布局和生命周期单独核验。

本地 JPEG/PNG 经 WIC 完整解码，读取自有快照，按30000字节分块发送。
每块校验真实 BaseResponse 和确认偏移；最终独立 GetProfile 核对大小头像资源、账号、原昵称和签名。
仅忽略实测出现的 HTTP/HTTPS 协议差异，其余完整 URL 必须一致。
submitted、uploaded、verified 分别表示提交、全部字节业务确认及资料回读确认；只有三阶段均完成才成功。
超时不自动重试，模块 pin 与共享状态保留晚到回调生命周期。头像与昵称、签名共享同一 DLL 写入互斥锁。

用户授权 C:\1.jpeg：667×500 JPEG，2619字节。最终新编译生产路由诊断实测 ret=0、errCode=1、
submitted=true、uploaded=true、verified=true，2619/2619，原昵称和签名未变。
wire、图片解码、HTTP 非法参数、未验证阶段不报成功、写锁测试通过；GetSelfProfile.cpp 新对象编译通过。
未要求恢复头像，完整 Hook DLL 未构建替换部署；真实多块、换号和断网超时等场景未实测。
完整协议、ABI、失败调查、验证及复现边界见 [头像实现记录](edit-self-avatar-4.1.15.55.md)。
