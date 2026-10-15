# 修改头像 — 4.1.15.55 原生实现与验证

日期：2026-10-15。当前项目：D:\WeChatHook4.1.15.55。

## 接口及结果

`POST /api/upload_head_img`，JSON 参数 `filepath`（UTF-8、本机盘符绝对路径）。示例：

```json
{"filepath":"C:\\1.jpeg"}
```

支持可完整解码的 JPEG/PNG，文件 1～4,194,304 字节，宽高各 1～4096。
文件打开时不共享写入，读入自有快照，WIC 解码验证；上传始终使用该快照。
不自动压缩、裁剪、重编码，不从 HTTP 地址下载图片。

成功须同时满足原生传输成功、服务器 BaseResponse.Ret=0、各块确认进度准确、
最终头像地址有效、独立 GetProfile 确认大小头像地址、昵称/签名未变及当前账号未切换。

```json
{"ret":0,"errCode":1,"submitted":true,"uploaded":true,"verified":true,
 "total_bytes":2619,"acknowledged_bytes":2619,
 "retmsg":"avatar uploaded and verified from server profile",
 "errMsg":"avatar uploaded and verified from server profile"}
```

`submitted` 仅说明至少一个原生请求已交给调度器；`uploaded` 说明全部图片字节已获服务器业务确认；
`verified` 说明服务端本人资料回读也已匹配。失败 ret=-1、errCode=-1，保留已发生的阶段标志。
例如 uploaded=true/verified=false 不能当作未上传，应先查询资料，禁止自动重试。
HTTP 状态沿用项目约定为 200，通过 JSON 结果判断业务状态。

## 当前构建及 native scene

实际目标为 C:\Program Files\Tencent\Weixin\4.1.15.55\Weixin.dll：
AMD64，timestamp=0x6ACCBF76，SizeOfImage=0xC34F000，
SHA256=6fbbba536b04c0a24730710f540887a53f4eaa24398e4f94905d9b4a36148595。
生产调用前由 SelfProfileMemory 校验构建及登录账号；本次证据不适用于其它构建。

安装模块中未定位到可直接调用的 UploadHDHeadImg 专用请求类型，因此没有套用旧版上传入口。
先核验已存在的通用 CGI scene 布局，再创建自有 scene、替换此实例的打包/解析虚方法，
使用微信自己的 BaseRequest、账号会话、网络调度和完成回调。未修改模块代码或全局虚表。

| 项目 | 本次核验 RVA / 布局 |
|---|---|
| scene 分配大小 / 构造 / 原虚表 | 0xF0 / 0x49A500 / 0x8FDC488 |
| 配置模板 / CGI / URI | 0x8FD8670 前16字节 / config+0x10 / config+0x18 |
| native BaseRequest 准备 | 0x49A810(scene, mode, context)，与原 Pack 0x49A6D0 一致 |
| BaseRequest / 回调槽 | scene+0xB0 / scene+0x98 |
| IOBuffer 追加 / 数据 / 长度 | 0x6979C0 / 0x697A00 / 0x697A30 |
| service | *(manager+0x2C8)，manager=*(base+0xBB92FC0)，虚表0x8FCD388 |
| scene 注册 / 调度器 / 入队 | service 虚表+0x28 / 0x691E60 / 0x692200(ctx,scene+0x18) |
| 原生分配 / 释放 | 0x754C41C / 0x754C460 |

AvatarCgi 复制 RTTI 和5个虚方法到自有表，只替换该 scene 的析构、Pack、Parse。
Pack 首先调用原生 BaseRequest 准备函数，序列化真实 BaseRequest 后追加上传 PB；
Parse 从原生 IOBuffer 复制解密后的响应，不使用 GetProfile 的响应类型解析上传数据。
析构先恢复该实例的原虚表，再交给原生析构函数释放。
原生服务持有 scene，状态表及回调持有共享状态；模块 pin 防止晚到回调访问已卸载代码。
原生调度器执行网络处理；调用 HTTP 线程只等待事件，不从 UI 线程同步调用上传实现。
每块等待最多30秒，总上传检查120秒预算；一块超时不重试，晚到回调仍可安全完成。
这些等待并非取消机制，超时后的业务结果可能尚未确定。

使用同一适配器发起只读 CGI302 GetProfile，并由已有身份校验器核对真实账号，通过后才尝试上传。
实现允许的 CGI 仅为上传157和该只读诊断302，不能作为任意 CGI 通道。

## 上传协议和资料验证

CGI157，URI `/cgi-bin/micromsg-bin/uploadhdheadimg`；每块最多30000字节，顺序发送，不并发重试。

| Request 字段 | 编码 |
|---|---|
| 1 BaseRequest | 微信原生场景生成，非固定数据 |
| 2 TotalLen / 3 StartPos / 4 HeadImgType | varint：整张图片长度 / 本块偏移 / 1 |
| 5 HeadImgBuf | SKBuiltinBuffer，field1 本块长度，field2 本块原始字节 |
| 6 ImgHash | 整张图片 MD5 小写十六进制 |

Response field1 为 BaseResponse；field2 TotalLen、field3 StartPos 必须与本块预期终点匹配。
可选 field4 FinalImgMd5sum 若存在必须匹配；本次服务器未返回该字段，未将缺失说成摘要已验证。
field5/6 为大小头像 URL，最终块需两个字段均有效。严格拒绝重复或类型错误的已知字段。
公开协议与实现只作字段线索，本次原生业务返回和资料回读才构成目标构建的实测证据：
[UploadHeadImage.go](https://github.com/ThePeppy/wechat-api-ipad/blob/main/models/User/UploadHeadImage.go)。

独立 GetProfile 复用已核验的 CGI302，先核对 UserInfo 的本人账号，再读响应field3 UserInfoExt
中的field9/10大小头像 URL。实测上传返回 http://wx.qlogo.cn，而 GetProfile 为 https://wx.qlogo.cn；
只忽略 http/https 协议，比较其余完整主机、路径和查询串，不用域名相同或部分路径匹配替代。
最多5次只读回查（间隔1秒）；每次 native 查询仍有自己的30秒等待边界。
同时核对原昵称、签名以及最终本机账号。未直接下载 CDN 图片或做像素相等验证；服务器可能裁剪处理。
昵称、签名、头像共享 SelfProfileMutationMutex，在同一 DLL 内互斥；不能协调其它客户端或其它诊断 DLL。

## 代码及复现

- include/AvatarFile.h：文件快照、WIC 完整解码；需要 ole32、windowscodecs（MSVC pragma 已声明）。
- include/AvatarWire.h：分块 PB、业务进度和 URL 校验，使用项目现有 MD5。
- include/AvatarCgi.h：当前构建的自有原生 scene、异步 Pack/Parse/回调及生命周期。
- include/EditSelfAvatar.h：写入互斥、分块上传、业务状态与本人资料验证。
- include/EditSelfAvatarRoute.h、src/GetSelfProfile.cpp：生产 HTTP 注册和阶段结果。
- tools/avatar_test.cpp：wire、真实指定图片解码、URL 资源比较、HTTP 阶段区分及锁测试（不上传）。
- tools/avatar_native_test.cpp、tools/avatar_native_runner.py：只读 native adapter 和短时生产 HTTP 真上传诊断。

Zig c++17 诊断构建使用 -shared（不加 -static）、-Iinclude、
-lole32 -lwindowscodecs -luuid -lws2_32 -lcrypt32。
本轮 DLL：.review/avatar_native_test.dll；离线测试：.review/avatar_test.exe。
只读入口 RunProfile；显式 --upload 才通过 HTTP 路由上传 C:\1.jpeg，不恢复旧头像。
运行：python tools/avatar_native_runner.py --pid <实际主进程PID> --upload。
该诊断源固定使用本次用户授权的图片，不能在其它账号或环境无授权重复执行。

## 本轮验证结果

1. 源码检查：原生 ABI 静态证据使用本机 pefile/Capstone，记录于 .review/avatar-scene-virtuals.asm、
   avatar-native-buffer-abi.asm；不是 REA Evidence（深度分析提供器未配置）。
2. 离线测试新编译并运行通过：PB 长度/进度、重复与错误字段、不同资源拒绝匹配、文件解码、
   HTTP 非法参数无提交、uploaded=true/verified=false 时不报成功、共享写锁拒绝提交。
3. 自有 scene 的只读 GetProfile 实测通过，真实返回账号身份匹配。
4. 早期 ModUserInfo field8/9 + Oplog 尝试虽命令被接受，但未得到头像变化确认；该路径被弃用。
5. CGI157 早期诊断两次均得到 Ret=0、2619/2619；直接字符串比较 URL 失败。
   后续定位为 HTTP/HTTPS 规范差异，早期失败结果未被当作头像已验证成功。
6. 最终生产头文件新编译为诊断 DLL，在同一生产 Register/Upload HTTP 路由实测指定
   C:\1.jpeg（JPEG RGB 667×500，2619字节），ret=0、errCode=1、submitted=true、
   uploaded=true、verified=true、确认字节2619；服务端大小头像资源匹配，原昵称和签名未变。
   非法 JSON、缺字段、错误类型、相对路径、文件不存在以及持锁请求均无提交。
   结果 .review/avatar-production-result.json；进程保持响应。用户只授权该图片上传，未要求恢复旧头像。
7. GetSelfProfile.cpp 新对象编译通过。完整代理 Hook DLL 未构建替换部署；
   诊断 DLL 运行结果不能表述为已部署完整 version.dll。

未实测：超过30000字节的真实多块上传、接近上限图片、真实服务器拒绝、换号、断网及30秒超时后的晚到回调。
这些路径有校验/生命周期保护，但不能称为真实场景已验证。没有发消息、点赞或支付操作。
