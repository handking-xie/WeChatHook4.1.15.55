# IsLoginStatus — Weixin 4.1.15.55

记录日期：2026-10-15。适用 Windows x64 的已核验 Weixin.dll 构建。

## 问题与修正

原接口以 contact.db 句柄是否存在判断登录，并返回全局 SelfInfo 缓存。
数据库句柄和缓存不能证明本次请求的当前账号已登录，可能在退出、初始化或账号切换时失真。

现在由 include/LoginStatusMemory.h 读取原生账号对象，include/LoginStatusRoute.h
负责响应和注册，src/ContactsMgr.cpp 调用注册函数。每次读取均检查 PE 构建、账号虚表、
登录标志以及前后账号快照；资料读取复用 ProfileMemory 的安全内存和 UTF-8 字符串工具，
不调用其旧版 Read，不更改其它接口的旧版构建门槛。

## 独立定位证据

实际加载模块：C:\Program Files\Tencent\Weixin\4.1.15.55\Weixin.dll。
PE timestamp=0x6ACCBF76，SizeOfImage=0xC34F000，AMD64。
本次使用本机 pefile、Capstone 和只读 ReadProcessMemory；REA MCP 未连接，
本节为普通本机工具观察，不是 REA Evidence。

| 项目 | 新版位置与依据 |
|---|---|
| 全局 manager | module+0xBB92FC0；RVA 0x45D10 的原生 getter 从该全局取值 |
| manager 虚表 | RVA 0x8F514E8，当前进程观察；不作为通用跨版本布局 |
| account | manager+0x68；manager 虚表 +0x60 的 RVA 0x469A0 返回该 shared_ptr，+0x70 为控制块 |
| account 虚表 | RVA 0x8FD1498；构造函数 0x37B880 中的 0x37B8B5 加载并写入它 |
| 登录位 | account+0x38 的 bit 0；虚表首项 RVA 0x389A50：movzx eax,byte ptr[rcx+0x38]; and al,1; ret |
| wxid / alias | account+0x48 / +0x68；虚表 getter RVA 0x3855C0 / 0x3855D0 返回字段地址 |
| nickname | account+0x88；沿账号对象核对的相邻 MSVC 字符串，与既有资料布局一致；未做独立界面核对 |

调查阶段在模块 .data 范围内查找指向候选 manager 的指针，以 account 虚表过滤，
再用原生 getter、构造函数和运行时对象交叉核对。生产代码只有固定指针链读取，
不扫描内存、不查询数据库、不读取账号目录、不访问 SelfInfo。
运行时登录位为 01，wxid/alias/nickname 字节长度分别为 19/14/6；不在记录中保存个人值。

## HTTP 合约

POST /IsLoginStatus，无参数。保留原有 status 和 is_login 字段。

- 已登录：ret=0、retmsg=success、status=1、is_login=true，并返回本次读取的 wxid、alias、nickname。
- 未登录：ret=0、retmsg=success、status=0、is_login=false，不读取或返回残留账号资料。
- 构建不匹配、指针为空/不可读、虚表错误、字符串异常或快照改变：
  ret=-1、具体 retmsg、status=-1、is_login=null，不返回资料。

调用方必须先检查 ret；null 表示未知，不能转换成已确认未登录。
空 manager/account 仅证明对象不可用，当前保守地作为读取失败，不猜测登录状态。
读取完成前再次核对 manager/account/虚表/登录位，并在登录时复读三个资料字段。
这只是尽力而为的一致性检查，不持有微信锁，无法排除 ABA 切换或完整生命周期竞争。
只实现并注册 HTTP POST；未新增或验证 WebSocket 分发。
本次仅适配 IsLoginStatus，不代表整套接口或旧版 ProfileMemory 已支持新版。

## 验证方式

工具 tools/login_status_test.cpp 直接调用生产 Read、Handle 和 Register。
无参数：稀疏模拟映像测试，覆盖登录/退出、其它标志位、错误构建、错误虚表、
非法 UTF-8、超长/不可读字符串、空 wxid、空/不可读 manager、空 account、输出清空；
并在随机本机端口验证真实 POST 请求的登录/退出/错误响应。
提供 PID 和实际模块基址：以 PROCESS_VM_READ | PROCESS_QUERY_INFORMATION 打开现有微信，
复用生产 Read，并通过相同生产 Register 的短时本机 HTTP 服务读取同一目标。
该诊断不注入 DLL、不修改微信内存、不提交网络业务请求；HTTP 服务结束后关闭。

编译示例（使用项目已有 Zig 0.16.0，先将缓存目录设为 .review/zig-global 和 .review/zig-local）：

```powershell
.review/compiler/ziglang/zig.exe c++ -std=c++17 -Iinclude tools/login_status_test.cpp -o .review/login_status_test.exe -lws2_32 -lcrypt32
.review/login_status_test.exe
# 每次重新确认 PID 和基址，不复用历史绝对地址：
$p = Get-Process Weixin | Where-Object { $_.Modules.ModuleName -contains 'Weixin.dll' }
$m = $p.Modules | Where-Object ModuleName -eq 'Weixin.dll'
.review/login_status_test.exe $p.Id $m.BaseAddress.ToInt64()
```

完整项目 core.dll 尚未构建和替换部署。诊断 HTTP 服务复用注册函数及读取函数，
但不等同于完整 Hook DLL 部署后的端到端验证。
真实退出/重新登录、账号切换、其它构建和高并发生命周期变化尚未实测。

### 本次结果

- Zig 0.16.0 Windows x64 测试程序编译通过；模拟读取和真实 HTTP 的登录、退出、未知状态测试全部通过。
- 重新枚举当前微信 PID/模块基址后，生产读取与短时本机 HTTP POST 验证通过，logged_in=1。
- ContactsMgr.cpp 对象编译通过，仅有既有第三方字面量运算符警告。
- Apifox JSON 解析与 git diff --check 通过。
