# EC34 Bolt-only 蓝牙键盘开发方案

生成时间：2026-05-30  
工作目录：`D:\Inuitive\EC34\EC34bolt`

## 最终目标

开发一个基于 E73/nRF52840 的 EC34 固件，使键盘可以直接配对并连接 Logi Bolt USB receiver，作为 Bolt receiver 下的无线蓝牙键盘工作。

本方案只把 Bolt 作为最终目标：

- Unifying 不作为主线，只保留为历史参考。
- Craft 身份只解释 Xorlink 在 Unifying/Options 下的现象，不作为 Bolt 主目标。
- Bolt 方向优先参考真正的 Bolt 键盘身份，例如 `MX Keys for Business` / `B363`、`MX Mechanical` / `B366`。
- 不以现有 ZMK 日常固件为第一实验对象，避免破坏已完成的 EC34 可用固件。

## 当前本机接收器状态

本机当前能看到的是 Unifying receiver：

- VID/PID：`046D:C52B`
- Windows 设备名：`Logitech Unifying USB receiver`
- 已配对设备里能看到 `QID_4003` 键盘和 `QID_1017` 鼠标。

这次没有枚举到 Bolt receiver：

- 预期 Bolt receiver VID/PID：`046D:C548`
- 后续需要插入 Bolt receiver 后再跑 receiver discovery 实验。

## 已确认的 Bolt 关键事实

### 官方层面

Logi Bolt 基于 Bluetooth Low Energy，但不是普通 BLE dongle。罗技官方资料强调：

- Bolt 使用 BLE 5.0 或更高。
- Bolt receiver 是闭环系统，只连接 Logi Bolt 设备。
- 键盘与 receiver 配对时使用 Security Mode 1 Level 4，也就是 authenticated LE Secure Connections。
- 键盘配对需要 6 位 passkey。
- 一个 Bolt receiver 最多可配对 6 个设备。

### Solaar 公开实现层面

Solaar 的 Bolt receiver 主机侧实现提供了非常关键的接口信息：

- Bolt receiver USB PID：`0xC548`
- Bolt receiver 使用 HID++ vendor 通道。
- receiver discovery 寄存器：`BOLT_DEVICE_DISCOVERY = 0xC0`
- receiver pairing 寄存器：`BOLT_PAIRING = 0x2C1`
- Bolt unique id 寄存器：`BOLT_UNIQUE_ID = 0x02FB`
- discovery notification：`DEVICE_DISCOVERY_NOTIFICATION = 0x4F`
- discovery status notification：`DISCOVERY_STATUS_NOTIFICATION = 0x53`
- pairing status notification：`PAIRING_STATUS_NOTIFICATION = 0x54`
- passkey request notification：`PASSKEY_REQUEST_NOTIFICATION = 0x4D`
- receiver discovery 阶段会拿到：
  - device kind
  - device BLE address
  - device authentication
  - device name
- keyboard pairing entropy 在 Solaar 中使用 `20`，mouse 使用 `10`。

这说明开发难点在设备侧：E73 固件必须以某种 Logitech/Bolt receiver 能 discovery 到的 BLE 设备形式出现，然后完成 authenticated pairing。

## 为什么不再以 Craft 为 Bolt 目标

用户确认：Xorlink 识别为 Craft 是通过 Unifying 实现的；连接 Bolt 时不是 Craft。

这与公开资料吻合：

- Craft Advanced Keyboard 是 HID++ 4.5 设备，WPID `4066`，BTID `B350`。
- Craft 是 Options/HID++ 兼容性的好参考，但不是典型 Bolt 设备。
- 真正 Bolt 键盘更适合参考：
  - `MX Keys for Business`，ID `B363`
  - `MX Mechanical`，ID `B366`

因此 Bolt-only 开发不要把 Craft 当最终目标。Craft 只作为“Options 如何识别 HID++ 设备”的历史参考。

## 推荐技术栈

### 主线：nRF5 SDK / SoftDevice 风格独立固件

原因：

- Xorlink 固件包是 Nordic DFU zip，使用 nRF Device Firmware Update App 更新。
- Nordic 官方说明 nRF Device Firmware Update App 面向 nRF5 SDK Secure DFU / Legacy DFU，不是 Zephyr/NCS 固件更新路线。
- Xorlink 的 app/boot `.bin` 高熵，像加密或混淆后的 DFU payload，不像普通 ZMK UF2。
- Bolt 是 BLE，但需要自定义 HID++ vendor 通道和 receiver discovery 行为；用 nRF5 SDK HIDS 示例做原型更可控。

建议基底：

- nRF5 SDK 17.x
- S140 SoftDevice
- Secure DFU bootloader
- `ble_app_hids_keyboard` 作为起点
- 加入 Logitech HID++ vendor report channel
- 后续再接入 EC34 的电容扫描和 keymap

### 备选：Zephyr 独立应用

Zephyr 也可行，尤其是 BLE 安全配置和 HID GATT 都成熟。但如果目标是复刻 Xorlink 风格和手机 nRF DFU 更新，nRF5 SDK 更接近。

### 不建议第一步直接改 ZMK

ZMK 可以保留作为 EC34 日常可用固件，但 Bolt 目标的不确定点太多：

- Bolt discovery 是否接受标准 ZMK BLE HID 未知。
- ZMK 默认 HID report map 没有 Logitech HID++ vendor 通道。
- Options/Bolt receiver 可能需要 HID++ 4.5 设备身份。
- 一旦直接改 ZMK，容易同时破坏键盘日常可用性和实验可控性。

## 开发阶段设计

### Phase 0：接收器观测

目标：确认 Bolt receiver 在当前机器上可访问，拿到 discovery/pairing 可观测数据。

操作：

1. 插入 Logi Bolt receiver。
2. 确认 Windows 枚举出 `046D:C548`。
3. 使用 Logi Web Connect 进入 Add Device。
4. 后续最好用 Linux + Solaar 做更细日志：
   - `solaar show`
   - `solaar -ddd pair`

本阶段只看 receiver 行为，不改键盘固件。

关键输出：

- receiver 是否为 `C548`
- receiver 是否能启动 discovery
- discovery 阶段的失败原因
- 是否有 passkey request
- Windows 设备枚举中是否出现 `QID_B363` / 类似 Bolt 键盘 ID

### Phase 1：最小 BLE HID + LESC/passkey 原型

目标：验证 Bolt receiver 是否会 discovery 一个标准 BLE HID keyboard。

固件特征：

- BLE HID keyboard
- Appearance keyboard
- HID service UUID
- Battery service
- Device Information service
- LE Secure Connections
- MITM/passkey
- Security Mode 1 Level 4 尽量满足

预期结果：

- 如果 receiver 能看到设备，继续进入 Phase 2。
- 如果 receiver 完全看不到，说明需要 Logitech/Bolt 特定 advertising 或 GATT/HID++ 身份。

### Phase 2：加入 Logitech HID++ vendor 通道

目标：让设备不仅是 BLE HID keyboard，还具备 Logitech HID++ 控制通道。

公开资料显示：

- Unifying/Bolt receiver 的主机 HID++ 通道常走 usage page `0xFF00`。
- Logitech 蓝牙直连设备的 HID++ 通道常见 usage page `0xFF43`、usage `0x0202`。
- HID++ short/long report id 常见 `0x10` / `0x11`。

设备侧要在 HID report map 中加入 vendor-defined report，并在固件里响应 HID++ 请求。

最小响应目标：

- `ROOT` / `0x0000`
- `FEATURE SET` / `0x0001`
- `DEVICE FW VERSION` / `0x0003`
- `DEVICE NAME` / `0x0005`
- `WIRELESS DEVICE STATUS` / `0x1D4B`
- `BATTERY STATUS` 或 `UNIFIED BATTERY`
- `CHANGE HOST` / `0x1814`
- `HOSTS INFO` / `0x1815`

先不追求 Options 完整配置，只追求 Bolt receiver 可配对、系统可输入。

### Phase 3：选择 Bolt 键盘身份 profile

目标：使用更接近真实 Bolt 键盘的公开身份模型。

候选 profile：

#### B363 / MX Keys for Business

优点：

- 明确是 Bolt 时代键盘。
- Solaar 资料里 feature set 清楚。
- Feature set 比 Craft 更贴近 Bolt receiver。

关键字段：

- Name：`MX Keys for Business`
- Protocol：HID++ 4.5
- Model ID：`B36300000000`
- Transport IDs：`btleid = B363`
- Battery：`UNIFIED BATTERY`

#### B366 / MX Mechanical

优点：

- 也是明确 Bolt 键盘。
- 公开 feature set 清楚。

缺点：

- reprogrammable keys 更多，键盘形态和 EC34 差异更大。

建议先用 `B363` 风格做互操作 profile。

注意：不要复制真实设备的唯一序列号、Unit ID、密钥或 bond 信息。只做公开层协议兼容，不做密钥克隆。

### Phase 4：Bolt 配对闭环

成功标准：

1. Logi Web Connect / receiver discovery 能发现键盘。
2. receiver 显示或触发 6 位 passkey。
3. 键盘端输入 passkey 并完成配对。
4. Windows 下出现 Bolt receiver 下面的新键盘设备。
5. 标准键码输入可用。
6. 断电重连后能自动回连。

如果卡住，按层定位：

- 发现不了：advertising / device name / HID++ vendor channel / Bolt-specific discovery 缺失。
- 发现但拒绝：device authentication / profile / identity 不匹配。
- passkey 失败：LESC/MITM/security level 或键盘输入处理问题。
- 配对成功但不能输入：HID report map 或 report routing 问题。
- 能输入但 Options 异常：HID++ feature set 不完整。

### Phase 5：接入 EC34 键盘功能

只有 Bolt 连接闭环成功后再接：

- EC34 电容扫描
- anti-drift
- keymap / layer
- battery ADC
- 蓝牙 LED
- DFU 快捷键
- 低功耗

这一步才开始移植现在 ZMK 固件里的键盘业务逻辑。

## 下一步立刻要做的事

### 1. 插入 Bolt receiver 后确认枚举

PowerShell 检查命令：

```powershell
Get-PnpDevice -PresentOnly |
  Where-Object { $_.InstanceId -match 'VID_046D|C548|BOLT|LOGI|LOGITECH' -or $_.FriendlyName -match 'Logi|Logitech|Bolt' } |
  Select-Object Class,FriendlyName,InstanceId,Status |
  Format-List
```

预期看到：

- `VID_046D&PID_C548`
- `Bolt Receiver`
- vendor-defined HID device

### 2. 用 Logi Web Connect 做空跑

不连接键盘，先只确认 receiver discovery 能启动：

- 打开 `https://logiwebconnect.com`
- 选择 Bolt receiver
- 进入添加设备
- 观察页面提示和超时行为

### 3. 准备最小 BLE probe 固件

先做一个和 EC34 无关的 E73/nRF52840 BLE HID 键盘 probe：

- 只发一个固定按键或不发键。
- 重点是 advertising、pairing、passkey、HID++ vendor channel。
- 成功后再移植 EC34。

## 结论

现在主线应改为：

`Bolt receiver discovery` → `BLE LESC/passkey` → `HID++ vendor channel` → `B363-like Bolt keyboard profile` → `keyboard input` → `EC34 integration`

不要再把 Unifying/Craft 当最终目标。Unifying 证明 Logitech 设备端模拟有公开基础；Craft 证明 Options 识别靠 HID++ 身份；但 Bolt 最终要走的是 BLE + Bolt receiver discovery + Bolt 键盘 profile。
