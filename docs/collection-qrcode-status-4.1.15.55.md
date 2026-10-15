# Busif2fgetqrcodev2 — 4.1.15.55 定位记录

日期：2026-10-15。**尚未完成原生实现，未构建或部署该接口的新 DLL。**

## 已核对的外部合约

来源为本项目 Apifox 集合“个人信息 / 获取面对面收款二维码”，以及其引用的
[ShowDoc 页面](https://www.showdoc.com.cn/PCWeixinHook/11559060627323000)。
本轮读取页面原始内容成功：该文档声明适用 4.1.10.27，POST /Busif2fgetqrcodev2，
空对象请求，返回 qrcode_url（wxp://f2f…）和脱敏姓名 name；没有提供内部 CGI 编号、
PB 请求/响应字段、支付风控参数或原生入口。它不能证明 4.1.15.55 已支持。

## 本次新增静态证据

REA open_binary 验证目标：C:\Program Files\Tencent\Weixin\4.1.15.55\Weixin.dll，
AMD64，SHA256 6fbbba536b04c0a24730710f540887a53f4eaa24398e4f94905d9b4a36148595。
后续 REA search_strings 返回 provider_unavailable：Ghidra 未配置、Hopper 不支持 Windows、
IDA 未配置。以下反汇编来自本项目现有 pefile/Capstone 工具，不属于 REA Evidence。
构建门槛沿用本项目已核验记录：timestamp=0x6ACCBF76、SizeOfImage=0xC34F000。

| 观察 | 位置与证据 | 可得结论及边界 |
|---|---|---|
| 通用 TenPay URI | RVA 0x99D0CE0，引用 0x53BD8C1、0x53C8332 | 存在支付传输场景，不证明能生成收款码 |
| TenPayRequest 名称 | RVA 0x9C7D858，引用函数 0x693A380 | protobuf 名称输出，不是收款码请求构造函数 |
| TenPayResponse 名称 | RVA 0x9C7D8D8，引用函数 0x693B340 | protobuf 名称输出，不是收款码响应字段定义 |
| 异步场景提交候选 | RVA 0x53C82B0，调用点 0x53C8002 | 静态观察到请求复制、回调转移、service 虚表 +0x28 注册和后续队列调度；未做运行调用 |
| 固定 CGI 编号 | 0x53C82F0 写入 0x181=385；0x53C869B–0x53C86A2 将该配置 +0x10 写入 scene+0xC | 该封装没有在本次已解码路径中提供替换 CGI 编号的参数，不能仅换 URI 当成目标业务 |
| URI 可被覆盖 | 0x53C8341 起从 options+8 的字符串复制；0x53C832F 默认 TenPay URI | 覆盖 URI 不等于覆盖 CGI 编号或 PB 类型 |
| 场景布局 | 构造 RVA 0x53C85F0，vtable RVA 0x99D09D8，request+0xA8、response+0xD8 | 只作为该构建的静态调查线索，未确认用于收款码 |
| 红包映射 | 构造 RVA 0x53CF350 | 映射 receivewxhb=1581、openwxhb=1685、qrydetailwxhb=1585；不能替代收款码请求，未调用 |
| PC 支付授权 URI | RVA 0x959A900，初始化函数 0x2DCD780 | 是 pcpayauthorize 字符串初始化，不能作为收款码原生提交函数 |

更宽的字节搜索覆盖版本目录顶层 .dll/.exe/.bin，同时查询 ASCII/UTF-16LE 形式。
对 busif2f、f2fget、f2fqrcode、collectqrcode、collect_qrcode、receiverqrcode 等专用词
未取得可用专用入口；存在支付域名、转账、红包及 PC 支付相关线索。
此结论仅限明文名称搜索，不能证明专用功能不存在，未覆盖压缩资源、动态拼接或全部运行时路径。
没有把普通个人二维码 /cgi-bin/micromsg-bin/getqrcode 当成收款码。

## 可复现工具与本次验证层次

新增 tools/collection_qr_survey.py：只使用 Python 标准库，支持 --module-dir 和 --limit；
列出每个顶层二进制的 SHA256、PE 元数据、各模式总命中数和截断标记，并将命中映射为 RVA。
不读取微信进程或账号数据，不注入 DLL，不调用支付网络接口。

```powershell
python tools/collection_qr_survey.py --module-dir 'C:\Program Files\Tencent\Weixin\4.1.15.55' > .review/collection-qr-survey.jsonl
```

本次 .review 中保存了 collection-showdoc-page.json、collection-qr-survey-final.jsonl、
tenpay-entry-refs.txt、tenpay-scene-ctor.asm、tenpay-scene-layout.asm、tenpay-async-submit.asm，
以及红包和 PC 支付授权调查输出。这些是本机调查产物；可复用结论及边界以上表为准。
未进行该业务的 native codec 测试、异步提交或真实收款码验证；没有支付、转账、领红包。
生产路由保留原有明确失败，未把候选入口或固定 URL 接成成功路径。

## 阻塞点

尚缺目标业务的 CGI 编号、PB 编解码及风控字段证据，以及能够承载该业务的原生场景。
用户确认没有可用旧版 DLL、请求样本或完整协议；本项目现有旧版 NativeCgi 偏移不能直接跨版复用。
需要进一步恢复专用或通用原生请求路径，再验证字段、ABI、线程和异步生命周期，
最终检查原生完成状态、服务端业务码和当前账号关联后才可返回真实 qrcode_url/name。
当前状态仍为“未实现”，本记录不计作已完成的接口实现。

本轮工具检查：扫描版本目录顶层 28 个二进制文件完成；上述六个专用词在两种编码下总命中为 0。
Weixin.dll 的 SHA256、AMD64、timestamp 和 SizeOfImage 与目标记录一致。
Python 语法编译和 git diff --check 通过；这些仅验证调查工具及文档，不是接口运行成功。
