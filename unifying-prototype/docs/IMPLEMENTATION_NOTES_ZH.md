# Unifying 原型实现笔记

## 当前实现范围

已经落地的部分：

- K270-like 初始键盘 profile。
- pairing request 1/2/3 帧构建。
- pairing complete 帧构建。
- pairing response 1/2/3 基础解析和 checksum 校验。
- Unifying 简单 checksum。
- keep-alive 帧构建。
- short/long wake-up 帧构建。
- plain keyboard report 帧构建。
- radio 抽象接口。
- 可选 Zephyr/NCS ESB radio adapter 草案：`src/ec34u_radio_esb_zephyr.c`。
- 阻塞式 bring-up 配对状态机：`ec34u_pair_with_receiver()`。
- 主机侧 C52B receiver HID++ short/long collection 探测工具。

尚未实现的部分：

- nRF52840 radio / ESB 实际收发。
- link key 派生。
- encrypted keyboard report。
- HID++ 查询响应。
- 持久化配对信息。
- EC34 电容扫描接入。

## 为什么先用 K270-like profile

第一次联通阶段需要尽量减少变量。`WPID 0x4003` 是公开资料中常见的 K270 HID++ 2.0 键盘身份，公开样例多，receiver 和主机侧都容易识别。

等配对和按键输入跑通后，再考虑更贴近 Xorlink 的身份或更完整的 HID++ profile。

当前机器的 C52B receiver 里本来就配着一个真实 `K270`，只读扫描结果为：

- WPID：`4003`
- Protocol：`04`
- Device flags：`0D`
- Extended pairing info 中可见类似 `1A40` 的能力字段
- 名称：`K270`

因此 EC34U 第一版 profile 使用：

- `EC34U_K270_WPID = 0x4003`
- `EC34U_K270_DEVICE_FLAGS = 0x0D`
- `EC34U_K270_PAIRING_CAPS = 0x1A40`

这些值不是最终产品身份，只是用于第一阶段减少变量。

## 当前 receiver 侧验证

已确认：

- `046D:C52B` 暴露 HID++ short collection：`UsagePage 0xFF00 / Usage 0x0001`，report 长度 7。
- `046D:C52B` 暴露 HID++ long collection：`UsagePage 0xFF00 / Usage 0x0002`，report 长度 20。
- short 请求 `10 FF 81 02 00 00 00` 可读 receiver connection。
- long register `RECEIVER_INFO 0x2B5` 需要向 short collection 写 `10 FF 83 B5 ...`，再从 long collection 读 `11 FF 83 B5 ...` 响应。
- 当前 receiver serial：`8CA59C8D`。
- 当前最大设备槽：`6`。
- 当前已配对设备：
  - Slot 1：`1017`，`Anywhere MX`
  - Slot 2：`4003`，`K270`

可重复验证命令：

```powershell
powershell -ExecutionPolicy Bypass -File "D:\Inuitive\EC34\EC34bolt\tools\Get-LogitechUnifyingSummary.ps1" -Vid 046D -ProductId C52B
```

## radio 层选择

优先级建议：

1. nRF5 SDK / direct radio / ESB 风格实现
   - 更接近 Xorlink 的技术路线。
   - 更容易控制 2.4GHz proprietary radio。
2. Zephyr ESB 或 direct RADIO
   - 更接近当前 ZMK 环境。
   - 但需要确认本地是否有完整 Zephyr/NCS 构建工具和 ESB 支持。

已添加 `ec34u_radio_esb_zephyr.c`，但默认只有在 `CONFIG_EC34U_UNIFYING_ESB` 打开时才启用。它当前作为 bring-up 草案存在，不会影响现有 ZMK 固件。

这个 adapter 按 Nordic/NCS ESB API 设计：

- `ESB_PROTOCOL_ESB_DPL`
- `ESB_MODE_PTX`
- `ESB_BITRATE_2MBPS`
- `ESB_CRC_16BIT`
- 5-byte address
- ACK enabled
- manual TX
- payload 最大至少 22 字节

需要实机验证的关键点：

- ESB 地址 byte order 是否和 Logitech receiver 完全一致。
- ACK payload 是否能正确收进 `ec34u_esb_rx_queue`。
- BLE/ZMK 同时启用时 radio 资源是否冲突。

## 安全边界

本原型只做用户自有 receiver 的正常配对。不要加入：

- 强制配对。
- 嗅探配对密钥。
- 针对未知 receiver 的注入。
- 从其他设备提取/复制 link key。

## 下一步具体任务

1. 查明本地构建环境是否能编译 nRF52840 radio demo。
2. 实现 `ec34u_radio_ops` 的 nRF52840 版本。
3. 在 receiver 正常配对模式下发送 pairing req1，确认是否收到 response 1。
4. 收到 response 1 后切换 receiver 分配的新 RF address。
5. 完成 req2/req3/complete，生成并保存 link key。
6. 实现 encrypted keyboard report 后，再接 EC34 电容扫描。

`ec34u_pair_with_receiver()` 已经把 3/4/5 中的配对帧顺序串起来了，但 link key 派生仍是 TODO，因此它目前的用途是验证 radio 收发和 receiver 是否接受 EC34U profile。
