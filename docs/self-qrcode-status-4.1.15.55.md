# GetQRCode — Weixin 4.1.15.55 原生实现

日期：2026-10-15。POST /GetQRCode，用于当前登录账号的本人二维码。

## 使用及返回

请求体留空或发送 {}。拒绝其它 JSON 类型、人工 QRCode_URL/qr_url、目标账号、opcode、
ret_ori_json/ret_ori_pb 等所有非空对象参数；不暴露重置操作，不接受外部二维码。

成功返回 ret=0、retmsg=success、submitted=true、verified=true、QRCode_URL 和兼容 qr_url。
QRCode_URL 来自本次服务端响应，不使用全局缓存、文件缓存、数据库、内存扫描或参数回显。
失败 ret=-1、具体 retmsg，并保留 submitted/verified，不返回二维码链接。
verified 指原生传输和 BaseResponse 业务成功、URL 格式检查以及请求前后账号检查通过；
没有将独立扫码解码、手机客户端核对或界面比对算入 verified。

未登录、构建或账号布局不匹配、原生服务不可用、业务失败、链接缺失/非法、账号变化均失败。
原生请求等待最多 30 秒，不自动重试；超时 submitted=true、verified=false 表示请求可能仍在运行。

## 文件及调用链

- include/SelfQrRoute.h：生产 HTTP 注册、只接受空对象、失败不发布 URL；允许注入 fetcher 测试。
- include/SelfQrNative.h：当前账号读取、串行请求、服务端校验及结束后的账号核对。
- include/SelfQrCgi.h：独立新版原生 ABI、request 构造/解析、scene 提交、回调及序列化。
- include/SelfQrWire.h：PB 请求编码、BaseResponse 和 field 9 URL 严格解析。
- src/GetSelfProfile.cpp：调用 self_qr::Register，替换之前的未实现保护。

旧版 NativeCgi.h 和 ProfileMemory.h 的支持范围没有扩大；新版仅复用安全内存工具，
使用独立 SelfQrCgi，不能据此认定其它 CGI 已适配。

```text
HTTP 空对象校验
 → 检查 4.1.15.55 构建和当前账号
 → GetQRCodeRequest：当前 wxid，opcode=0，style=0
 → 原生 ParsePartialFromString；BaseRequest 由 scene 补充
 → 再核对账号，异步提交 CGI 168
 → 完成回调获取 scene 响应并序列化为自有字节串
 → BaseResponse.Ret=0，解析第 9 字段微信个人二维码 URL
 → 复核当前账号未变，返回 QRCode_URL
```

## 当前构建入口与证据

目标 C:\Program Files\Tencent\Weixin\4.1.15.55\Weixin.dll；AMD64，timestamp 0x6ACCBF76，
SizeOfImage 0xC34F000。SHA256：
6fbbba536b04c0a24730710f540887a53f4eaa24398e4f94905d9b4a36148595。
构建门槛由 SelfProfileMemory::Read 核对，不将文件夹名当作兼容依据。

本轮使用项目 .review/login_analysis.py 的 pefile/Capstone、只读对象核对及自有诊断 DLL；
反汇编是普通本机工具观察，不能称为 REA Evidence。旧版有限 MOV/LEA 搜索未发现引用，
这次 broadrefs 检查相对位移并人工核对完整指令，找到实际 URI 引用。

| 项目 | RVA / 布局 |
|---|---|
| URI | 0x9299080，/cgi-bin/micromsg-bin/getqrcode |
| 异步提交 | 0x1726630，CGI 168；0x1726670 写入 0xA8 |
| scene 构造 / 虚表 | 0x1726970 / 0x9298D78；分配 0x140 字节 |
| scene 请求 / 响应 | +0xA8 / +0xD8 |
| 请求构造 / 对象大小 / 虚表 | 0x382BA90 / 48 字节 / 0x96D0338 |
| 响应构造 / 虚表 | 0x382BAC0 / 0x96D0448 |
| 请求复制 | 0x382C6F0，由异步提交函数在注册前调用 |
| 部分解析 | 0x697160；读取 native string，并调用 message 虚表 +0x18 清空和 +0x38 解析 |
| 序列化 | message 虚表 +0x40 计算长度、+0x50 写入自有缓冲区；通用实现 0x697310 |
| 原生分配 / sized free | 0x754C41C / 0x754C460；request 通过原生虚析构释放 |
| manager / service | *(base+0xBB92FC0)，service=*(manager+0x2C8) |
| CGI service 虚表 / 注册 | 0x8FCD388 / +0x28（0x35CA20） |
| 原生队列调度 | 0x691E60 / 0x692200 |

service 虚表已在当前进程只读核对；构造引用位于 0x356D9A、0x35F743。
0x35CA20 在 +0x68 获取原生 mutex，注册场景后由原生队列调度。
普通 HTTP/诊断线程使用该异步提交入口，不调用需要原生 coroutine 上下文的同步包装。

## PB 字段与异步所有权

请求：field 1 BaseRequest（原生 scene 补充）、field 2 SKBuiltinString_t（嵌套 field 1 当前 wxid）、
field 3 opcode=0、field 4 style=0。请求序列化函数 0x382C4B0 与 native codec 往返证据相符。
不传非零 opcode；没有测试或支持重置二维码。

响应序列化函数 0x382DD30：field 1 BaseResponse、field 2 图片缓冲对象、field 9 直接字符串。
当前真实响应业务码为 0；field 2 包含 17472 字节图片，field 9 是 44 字节微信个人二维码链接。
生产返回仅使用 field 9，不将图片字节猜测成 URL，也不回传完整原始 PB。
链接仅接受经过检查的微信个人二维码前缀和可打印 ASCII，拒绝控制字符、NUL、引号及超长值。
缺失或重复 BaseResponse、业务码、URL，错误 wire 类型、截断均失败；允许未知 PB 字段。

提交 ABI 为 submit(service, request, nativeWxidString, options, functionStorage)。
场景复制请求并转移 MSVC std::function 的 +0x38 implementation；请求在提交返回后原生析构。
options 为 48 字节；native string 为 32 字节、SSO 容量 15；function storage 为 64 字节。
回调 ABI 为 bool(implementation, scene**, errorType*, errorCode*)，通过 scene 虚表 +0x20 取响应。
回调把响应复制到自有缓冲区，不保留 scene 的内部指针。
共享完成状态保留到原生回调释放；模块 pin 到进程结束，超时后晚到回调不会执行已卸载代码。
超时不取消原生场景。串行锁仅约束本 DLL 的并行查询，客户端其它操作不受该锁保护。
账号前后读取是尽力而为检查，不排除 ABA 切换或完整原生对象生命周期竞争。

## 验证与部署边界

1. tools/self_qr_test.cpp：生产 wire 和真实本机 HTTP 注册函数测试通过。
   覆盖成功、未知 PB 字段、业务拒绝、缺失/重复/截断字段、非法 URL、超长/NUL、参数注入、
   超时状态、未验证结果及失败不泄露 URL；非法参数不调用 backend。
2. tools/self_qr_native_test.cpp / self_qr_native_runner.py：当前微信内 native codec 往返通过，
   使用生产请求编码，原生 request 解析、序列化及析构成功。
3. 原生查询收到完成回调，传输成功、BaseResponse.Ret=0，第 9 字段为微信 URL，长度 44。
4. 最终诊断 DLL 调用相同生产 Register/Fetch，在随机本机端口查询成功，ret=0、verified=true；
   人工 URL 参数请求被拒绝。短时 HTTP 服务测试后已停止。日志未保存链接、wxid 或图片内容。
5. GetSelfProfile.cpp 重新编译为 Windows x64 对象通过；接口集合 JSON 和 git diff --check 检查。

初次诊断构建误用 -shared -static，产物实际为归档库，加载未成功、没有执行原生查询；
修正为 -shared 后核对 PE 导入并本机加载成功，再做上述 native 测试。
有效诊断文件为 .review/self_qr_native_test2.dll（首次 codec/字段调查）和
.review/self_qr_native_final.dll（最终生产路由）。测试 DLL 保留在进程内，不卸载可能承载晚到回调的代码。

本次没有构建、替换部署完整 Hook DLL；诊断 DLL 的生产路由验证不能替代完整代理 DLL 部署验收。
真实退出重登、账号切换、高并发、网络超时及其它构建未运行实测。

```powershell
# 项目已有 Zig 0.16.0；缓存目录应设在当前项目 .review 内。
.review/compiler/ziglang/zig.exe c++ -std=c++17 -Iinclude tools/self_qr_test.cpp -o .review/self_qr_test.exe -lws2_32 -lcrypt32
.review/self_qr_test.exe
.review/compiler/ziglang/zig.exe c++ -shared -std=c++17 -Iinclude tools/self_qr_native_test.cpp -o .review/self_qr_native_final.dll -lws2_32 -lcrypt32
# 重新枚举 PID 后选定实际加载 Weixin.dll 的主进程；默认请求会真实查询本人二维码。
python tools/self_qr_native_runner.py --pid <当前主进程PID> --codec
python tools/self_qr_native_runner.py --pid <当前主进程PID> --http
```
