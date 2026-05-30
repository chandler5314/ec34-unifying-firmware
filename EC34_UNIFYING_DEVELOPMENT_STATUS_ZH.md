# EC34 Unifying 开发状态

更新时间：2026-05-31  
工作目录：`D:\Inuitive\EC34\EC34bolt`

## 当前目标

Bolt 暂停，先实现 EC34 通过 Logitech Unifying receiver 联通。当前只面向用户自己的 `046D:C52B` receiver，走正常配对流程，不做强制配对、嗅探密钥或未知 receiver 注入。

## 已验证的 receiver 事实

当前插入的 receiver：

- VID/PID：`046D:C52B`
- Receiver serial：`8CA59C8D`
- 最大设备槽：`6`
- HID++ short：`UsagePage 0xFF00 / Usage 0x0001`，7 字节 report
- HID++ long：`UsagePage 0xFF00 / Usage 0x0002`，20 字节 report

当前已配对设备：

| Slot | WPID | Protocol | Kind | Flags | Name |
| --- | --- | --- | --- | --- | --- |
| 1 | `1017` | `04` | mouse | `06` | `Anywhere MX` |
| 2 | `4003` | `04` | keyboard | `0D` | `K270` |

复测命令：

```powershell
powershell -ExecutionPolicy Bypass -File "D:\Inuitive\EC34\EC34bolt\tools\Get-LogitechUnifyingSummary.ps1" -Vid 046D -ProductId C52B
```

## 已新增工具

- `tools\List-LogitechHidCaps.ps1`
  - 枚举 receiver HID collection。
- `tools\Invoke-LogitechHidppReport.ps1`
  - 发送单条 HID++ report。
  - 支持 short collection 写入、long collection 读取。
  - 会按 request id 过滤响应，避免误读旧通知。
- `tools\Get-LogitechUnifyingSummary.ps1`
  - 读取 receiver info、设备槽、WPID、名称。

## 已新增设备端原型

目录：`unifying-prototype`

已实现：

- K270-like profile：
  - `WPID 0x4003`
  - `device flags 0x0D`
  - `pairing caps 0x1A40`
- pairing request 1/2/3
- pairing complete
- pairing response 1/2/3 解析
- Unifying checksum 计算/校验
- keep-alive
- short/long wake-up
- plain keyboard report
- `ec34u_radio_ops` 抽象接口
- `ec34u_pair_with_receiver()` 阻塞式 bring-up 配对状态机
- Zephyr/NCS ESB adapter 草案：
  - `unifying-prototype\include\ec34u_radio_esb_zephyr.h`
  - `unifying-prototype\src\ec34u_radio_esb_zephyr.c`

尚未实现：

- nRF52840 真实 2.4GHz radio/ESB 收发层
- link key 派生
- encrypted keyboard report
- HID++ 查询响应
- NVS/flash 保存配对信息
- EC34 电容键盘扫描接入 Unifying report

## 技术路线判断

Unifying 不是 BLE，也不是 ZMK 现有 HID over GATT。它需要 nRF52840 的 2.4GHz proprietary radio 跑 Unifying 风格收发。

更可行的第一阶段路线是独立 radio bring-up：

1. 做一个独立 nRF52840 Unifying bring-up 固件。
2. 只实现 radio、pairing、一个测试按键。
3. 让 receiver 在正常配对模式下发现 EC34U。
4. 确认 `Get-LogitechUnifyingSummary.ps1` 出现新槽位。
5. 再把 EC34 电容扫描、keymap、电池、LED 迁入。

ZMK 集成路线放到第二阶段，因为 ZMK 当前 BLE/USB 栈和 proprietary radio 的并存会增加调度与功耗复杂度。

## 下一个开发点

下一步应把 `ec34u_radio_ops` 的 nRF52840 实现从草案变成可编译固件：

- 2 Mbps
- 16-bit CRC
- 5-byte address
- dynamic payload
- auto-ack / ACK payload
- pairing address
- pairing channel hopping
- data channel hopping

当前已经有 Zephyr/NCS ESB adapter 草案，但本机没有可用的 `west/cmake/ninja`/ARM toolchain 环境，因此还没有做编译验证。

跑通标准：

- receiver 处于正常配对模式时，EC34U 发 pairing request 1。
- EC34U 收到 pairing response 1。
- 解析出 receiver 分配的新 RF address。

做到这一步，Unifying 联通路线就从“协议原型”进入“实机可验证”。
