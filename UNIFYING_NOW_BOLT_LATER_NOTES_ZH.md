# 当前无 Bolt 接收器时的 Unifying 临时验证记录

生成时间：2026-05-30  
工作目录：`D:\Inuitive\EC34\EC34bolt`

## 是否没有 Bolt receiver 就不能继续

不能做最终 Bolt 闭环验证。下面这些必须等 `046D:C548` Bolt receiver 在手：

- Bolt receiver discovery 是否能发现自制键盘。
- device authentication 字段是否被接受。
- 6 位 passkey 是否触发、是否能完成。
- 配对后 Windows/Options/Logi Web Connect 里是否出现 Bolt 键盘。
- 断电后是否自动回连。

但是可以继续做三类准备工作：

- 主机侧 Logitech HID++ 工具链验证。
- HID++ vendor channel 和 report 格式确认。
- 设备身份层资料整理，尤其是 Bolt 键盘 profile，例如 `B363` / `B366`。

现在插着 Unifying receiver，所以可以先走这条临时验证路线。它不会直接证明 Bolt 可行，但能验证很多后面 Bolt 也要用到的基础工具。

## 当前机器上的 Unifying receiver

Windows 枚举到：

- Receiver：`VID_046D&PID_C52B`
- 名称：`Logitech Unifying USB receiver`
- 已配对键盘：`QID_4003`
- 已配对鼠标：`QID_1017`

这说明当前机器有一个可用的 Logitech receiver 环境，适合做 HID++ 主机侧只读探测。

## 已添加的本地工具

脚本：

`D:\Inuitive\EC34\EC34bolt\tools\List-LogitechHidCaps.ps1`

用途：

- 只读枚举 HID 设备。
- 输出 VID/PID、UsagePage、Usage、Input/Output report 长度、DevicePath。
- 不写入 receiver，不打开 pairing，不改变配置。

运行命令：

```powershell
powershell -ExecutionPolicy Bypass -File "D:\Inuitive\EC34\EC34bolt\tools\List-LogitechHidCaps.ps1" -Vid 046D -ProductId C52B
```

## 关键发现

当前 `C52B` receiver 暴露了两个 Logitech HID++ vendor collection：

### HID++ short message

- VID/PID：`046D:C52B`
- UsagePage：`0xFF00`
- Usage：`0x0001`
- InputReportByteLength：`7`
- OutputReportByteLength：`7`

### HID++ long message

- VID/PID：`046D:C52B`
- UsagePage：`0xFF00`
- Usage：`0x0002`
- InputReportByteLength：`20`
- OutputReportByteLength：`20`

这与公开资料一致：

- Unifying/Bolt receiver 主机侧 HID++ 控制通道通常使用 `UsagePage 0xFF00`。
- short HID++ report 通常是 7 字节。
- long HID++ report 通常是 20 字节。

这个结果对 Bolt 有复用价值，因为 Solaar 也把 Bolt receiver `C548` 作为 Logitech receiver，通过 HID++ register 做 discovery 和 pairing。

## Unifying 这几天能做什么

### 可以做

- 枚举 receiver 和已配对设备。
- 验证 HID++ short/long 通道。
- 做只读 HID++ 查询工具，例如读取 receiver info、paired device info。
- 熟悉 WPID/QID、device kind、HID++ report 格式。
- 后续若需要，可以用 Unifying receiver 验证一个独立的 2.4GHz Unifying 原型。

### 不建议做

- 不建议把 Unifying 原型当成 Bolt 主线。
- 不建议为了 Unifying 花太多时间做完整设备端。
- 不建议操作 receiver pairing/unpair，避免影响现在已配对设备。

## 等 Bolt receiver 到手后的第一步

插入 Bolt receiver 后先跑：

```powershell
powershell -ExecutionPolicy Bypass -File "D:\Inuitive\EC34\EC34bolt\tools\List-LogitechHidCaps.ps1" -Vid 046D -ProductId C548
```

预期要看到：

- `UsagePage 0xFF00 / Usage 0x0001`
- `UsagePage 0xFF00 / Usage 0x0002`
- short/long HID++ report 长度

如果和 `C52B` 类似，下一步就可以写 Bolt receiver 只读探测：

- 读 Bolt receiver unique id。
- 读 receiver info。
- 启动 discovery 前先确认只读请求能正常收发。

## 当前结论

没有 Bolt receiver 时，不能验证最终目标，但不需要停工。现在最有价值的临时路径是：

`Unifying receiver 枚举` → `HID++ vendor 通道确认` → `只读 HID++ 工具` → `等 Bolt C548 到手后复用同一工具链`

这条路不会偏离最终目标，因为我们只把 Unifying 用作 Logitech receiver/HID++ 工具链验证平台。
