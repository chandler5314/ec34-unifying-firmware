# EC34 Logitech Unifying / Bolt 深入路线研究

生成时间：2026-05-30  
工作目录：`D:\Inuitive\EC34\EC34bolt`

## 当前判断

这条路和“在 ZMK 里改蓝牙配置”不是同一层问题。根据新线索和公开资料交叉验证，Xorlink / HHKB DMC 更像是一个基于 Nordic nRF5 SDK / SoftDevice / Secure DFU 的自研固件，而不是 ZMK 固件加一个开关。

高概率实现链路是：

1. 先实现 Logitech Unifying 设备端，也就是让键盘通过 nRF 的 2.4GHz proprietary radio 与 Unifying receiver 配对。
2. 在设备身份层模拟一个罗技键盘，用户反馈里提到是 Craft；Solaar 公开资料显示 Craft 的关键身份是 HID++ 4.5、WPID `4066`、Bluetooth ID `B350`。
3. 实现足够的 HID++ 1.0/2.0 响应，让 Logitech Options / Options+ 能识别设备、显示电量、支持 Flow 或部分配置能力。
4. 后续添加 Bolt 支持。Bolt receiver 侧公开资料显示它先 discovery，再收到设备 address / authentication / name，然后进入带 passkey 的 pairing。设备侧要能被 Bolt receiver discovery 到，这一步不是普通 BLE HID 能保证的。

所以真正的突破点不是 EC34 的键值和引脚，那些后面确实容易；难点是 Logitech receiver 无线协议和 HID++ 设备身份。

## 用户补充线索的意义

### 1. 先优联、后 Bolt

这个顺序非常合理。Unifying 的公开破解/研究资料多，且已经有设备端模拟样例；Bolt 公开资料少，但 Bolt 连接后仍会经过 Logitech receiver、HID++ 设备识别、Options 设备模型这些层。先完成 Unifying + HID++ 身份，再补 Bolt，是最自然的工程路线。

### 2. Options 识别成 Craft

这非常关键。它说明 Xorlink 不只是发送 USB HID 键码，而是在 Logitech 生态里暴露成某个已知设备。

Solaar 公开设备库里 Craft 的描述是：

- 名称：`Craft Advanced Keyboard`
- USB/Bluetooth ID：`046d:B350`
- Codename：`Craft`
- Protocol：`HID++ 4.5`
- Model ID：`B35040660000`
- Transport IDs：`btleid = B350`，`wpid = 4066`
- 支持特性包括：
  - `DEVICE FW VERSION`
  - `DEVICE NAME`
  - `WIRELESS DEVICE STATUS`
  - `BATTERY STATUS`
  - `CHANGE HOST`
  - `HOSTS INFO`
  - `REPROG CONTROLS V4`
  - `DFUCONTROL SIGNED`
  - `CROWN`

这解释了为什么 Options 会把它认成 Craft：不是因为键盘布局，而是因为设备报告了 Logitech HID++ 设备身份和特性集合。

不过这里要分清两件事：

- Craft 是一个很好的 Options/HID++ 兼容目标，公开资料完整，WPID/BTID/feature set 都容易对照。
- 如果目标是 Bolt receiver，Craft 未必是最佳设备身份，因为 Craft 不是典型的 Bolt 产品。

Solaar 资料里真正的 Bolt 键盘样例更像：

- `MX Keys for Business`
  - WPID / BTLE ID：`B363`
  - Protocol：HID++ 4.5
  - Model ID：`B36300000000`
  - Transport IDs：`btleid = B363`
  - Feature set 包括 `UNIFIED BATTERY`、`REPROG CONTROLS V4`、`CHANGE HOST`、`HOSTS INFO`、`BACKLIGHT2`、`DFUCONTROL`
- `MX Mechanical`
  - WPID / BTLE ID：`B366`
  - Protocol：HID++ 4.5
  - Model ID：`B36600000000`
  - Transport IDs：`btleid = B366`
  - Feature set 类似，也包括 `KEYBOARD LAYOUT 2`

因此可以把 Craft 看作“Options 识别和 HID++ 4.5 的已知目标”，而把 `B363/B366` 看作“Bolt receiver 更可能接受的设备身份候选”。

### 3. 用 nRF Device Firmware Update 更新

Xorlink 的发布包结构是 Nordic DFU zip：

- `*_app.bin`
- `*_app.dat`
- `*_boot.bin`
- `*_boot.dat`
- `manifest.json`

用户更新时用手机端 nRF Device Firmware Update App，键盘端按组合键进入 DFU。这与 Nordic nRF5 SDK Secure DFU / bootloader 模式高度一致。

Nordic 官方对 nRF Device Firmware Update 的描述也正好对应这一点：该 App 用 BLE transport 更新 nRF5 SDK 固件，目标是带 nRF5 SDK Secure Bootloader 或 Legacy Bootloader 的 nRF51/nRF52；官方还特别说明它不能用于更新 nRF Connect SDK 固件，NCS/Zephyr 应使用 nRF Connect Device Manager。

这意味着：Xorlink 固件大概率不是 Zephyr/ZMK UF2 流程，而是 Nordic nRF5 SDK + SoftDevice + Secure Bootloader 流程。它可以使用 SoftDevice 做 BLE，也可以通过 timeslot 或裸 radio 做 proprietary 2.4GHz。

## 已验证的公开资料

### bilogic/logitech-unifying-device

仓库：`https://github.com/bilogic/logitech-unifying-device`

这个工程明确写着“build Logitech Unifying compatible devices”。它使用 ESP8266 + nRF24L01+，目标是让自制设备与 Logitech C-U0007 Unifying receiver 配对。

它证明了几件事：

- Unifying 设备端模拟有公开工程样例。
- 配对不是一句“发键码”，而是多阶段 handshake。
- 设备需要报告 WPID、device type、capabilities、device name、report types。
- 连接后还要维持 keep-alive。
- 要处理 HID++ 1.0/2.0 查询。
- 键盘报告可以有加密路径。
- 电池状态也可以通过 HID++ 特性/寄存器提供给接收器和 Options。

这个项目使用外接 nRF24L01+，不是 nRF52840。但 nRF52840 的 2.4GHz radio 能做 proprietary radio/ESB，因此有移植可能。

### LOGITacker

仓库：`https://github.com/RoganDawes/LOGITacker`

这是 Logitech 无线输入设备的安全研究工具，不是产品固件，但它证明了 nRF52840 硬件路线可行。README 写明支持：

- Nordic nRF52840 Dongle `pca10059`
- MakerDiary MDK Dongle
- MakerDiary MDK
- April Brother nRF52840 Dongle

它覆盖 Unifying/Lightspeed/G700 等 2.4GHz 协议研究，具备发现设备、解析能力、配对相关研究、RF 帧处理、USB HID pass-through 等能力。

安全边界：LOGITacker 主要是安全测试工具，包含注入和漏洞利用能力。对 EC34 项目只能借鉴“nRF52840 能处理 Logitech 2.4GHz RF”和“协议组件存在公开研究”这两个事实，不应该直接搬攻击功能。

### Solaar

仓库：`https://github.com/pwr-Solaar/Solaar`

Solaar 是 Linux 下管理 Logitech Unifying/Bolt/Lightspeed/Bluetooth HID++ 设备的开源项目。它对我们最有价值的是接收器和设备识别层：

- `base_usb.py` 里列出 Bolt receiver USB PID `0xC548`，Unifying receiver `0xC52B` / `0xC532`。
- `receiver.py` 里能看到 Unifying 和 Bolt 接收器的配对流程是不同的。
- Bolt receiver 使用 discovery，再对发现到的 device address / authentication / name 调用 pairing。
- `notifications.py` 里能看到 Bolt discovery/pairing/passkey 相关通知。
- `descriptors.py` 里 Craft 是 `Craft Advanced Keyboard`，HID++ 4.5，WPID `4066`，BTID `0xB350`。
- `docs/devices/Craft Advanced Keyboard B350.txt` 给出了 Craft 的实际 feature set。

Solaar 是“主机/接收器侧”实现，不包含键盘设备端固件，但它告诉我们接收器期待怎样的信息，以及 Options/Solaar 怎么识别 Craft。

## 技术分层

### A. Radio / Link 层

Unifying：

- Logitech proprietary 2.4GHz，不是 BLE。
- 公开项目常用 nRF24L01+ 或 nRF52840 radio。
- 需要处理频道、地址、auto-ack、payload、checksum、keep-alive 等。
- nRF52840 可以用 proprietary radio / ESB 类方式实现。

Bolt：

- 官方说明基于 BLE。
- Bolt receiver 不是普通 BLE 主机，它通过 receiver 固件和 Logitech pairing 流程工作。
- 接收器侧公开流程有 discovery、pairing、passkey。
- 设备侧如何让 receiver discovery 到，公开资料仍不足。

### B. Pairing 层

Unifying：

- 公开项目显示有多阶段配对包。
- receiver 进入 pairing mode 后，设备端广播/响应，交换 WPID、地址、nonce、report types、device name 等信息。
- 键盘链路通常需要加密，配对阶段会派生或保存 link key。

Bolt：

- receiver 先进入 discovery。
- receiver 收到设备 kind/address/authentication/name。
- receiver 发起 pairing，键盘端显示或输入 passkey。
- 键盘连接成功后变成 Logitech receiver 下的一个设备。

### C. HID++ / Device Identity 层

这是让 Options 认设备的核心。

若模拟 Craft，至少要围绕这些信息构建：

- WPID：`4066`
- BTID：`B350`
- Model ID：`B35040660000`
- Protocol：HID++ 4.5
- Device name / friendly name
- Battery feature
- Wireless device status
- Host switching
- Reprogrammable controls
- Firmware version

Options 不一定要求完整实现 Craft 的全部 33 个特性，但如果某些特性被查询后没有合理响应，就可能显示异常、不能配置、甚至配对后掉线。

公开工具 `input-switcher` 和 Solaar 的资料还给了一个很有用的主机侧事实：

- Unifying/Bolt receiver 的 HID++ 控制接口通常走 vendor usage page `0xFF00`。
- Logitech 蓝牙直连设备的 HID++ 控制接口常见 usage page 是 `0xFF43`、usage `0x0202`。
- HID++ short/long message 常见 report id 是 `0x10` / `0x11`。

这说明 Logitech Options/Options+ 并不是只看标准键盘 HID report。它还会通过 vendor-defined HID++ 通道查询和控制设备。因此设备端若要被 Options 正常识别，除了普通键盘输入，还需要暴露并响应 HID++ 控制通道。

### D. Keyboard Application 层

这层才是 EC34 本身：

- EC 电容按键扫描
- anti-drift
- keymap
- 电池 ADC
- 蓝牙灯
- 层切换
- USB HID

这些在现有 ZMK 里已经基本完成，但如果走 nRF5 SDK 独立固件，需要重新移植一遍。

## 为什么 Xorlink 可能不是 ZMK

证据链：

- 固件发布包是 Nordic DFU zip，包含 app/boot `.bin/.dat`，不是 ZMK 常见 UF2。
- 手机用 nRF Device Firmware Update App 更新。
- 固件内没有明显 ZMK/Zephyr 字符串线索。
- 解包后的 `*_app.bin` / `*_boot.bin` 开头不是 Cortex-M 固件常见的向量表形态，整体熵接近 8.0，明显像加密或混淆后的 payload，而不是普通可反汇编裸 bin。
- 早期公开 `qmk_firmware_nrf52840` 仓库没有搜到 Logitech/Bolt/Unifying 实现。
- 支持 USB/BLE/Unifying/Bolt/Flow/Options 识别，这更像自研协议栈。

推断：Xorlink 可能是 nRF5 SDK + S140 SoftDevice + Secure DFU Bootloader + 自写键盘/Logitech 协议模块。

### 固件包反编译补充判断

三套 V1.7.0 固件的 application 和 bootloader 二进制都呈现高熵：

- `hhkb_bt_app.bin`：约 174 KB，熵约 7.999
- `hhkb_hybrid_app.bin`：约 175 KB，熵约 7.999
- `hhkb_pro2_app.bin`：约 176 KB，熵约 7.999
- `*_boot.bin`：约 45 KB，熵约 7.996

正常 nRF52 application bin 的开头通常能看到初始栈指针和 reset vector，地址会落在 RAM / flash 合理范围内；这里开头 32 字节看起来是随机数据。因此目前可以认为：公开下载包不适合走 Ghidra 直接反汇编，最多只能确认 DFU 包结构，不能提取可移植实现。

## 对 EC34 的可行路线

### 路线 1：保留 ZMK，只做 Bolt probe

用途：验证普通 BLE/ZMK 是否能被 Bolt receiver 发现。

优点：

- 风险最低。
- 不动现有可用固件。
- 很快能得到“能不能被 receiver discovery”的结果。

缺点：

- 成功概率不高。
- 即便能进 passkey，也不代表能被 Options 识别为 Craft。

适合下一步做的实验：

- 当前 ZMK BLE 进入 pairing，Logi Web Connect / Solaar Bolt pairing 是否能发现。
- 开启 ZMK passkey 后是否有变化。
- 观察 receiver 侧错误：没发现、device not supported、timeout 还是 passkey fail。

### 路线 2：Unifying-first 独立固件

用途：复刻 Xorlink 最可能的初始路线。

优点：

- 公开资料最多。
- bilogic 有设备端样例。
- LOGITacker 证明 nRF52840 radio 能跑相关 2.4GHz 协议研究。
- Solaar 提供 receiver/HID++/Craft 身份资料。

缺点：

- 基本不是 ZMK 插件级别，要写或移植一套 radio/link/pairing/HID++ 层。
- 如果用 nRF5 SDK，要重做 EC34 键盘应用层。
- 如果用 Zephyr，要解决 BLE 与 proprietary radio 共存、时序、功耗、USB、DFU 等问题。

建议架构：

- 第一阶段：nRF52840 上实现 Unifying device 最小闭环。
- 第二阶段：伪装为一个简单键盘，比如 K270/K800 级别身份，只做到可配对、可输入。
- 第三阶段：切换到 Craft 身份，实现必要 HID++ 4.5 特性，让 Options 识别。
- 第四阶段：再把 EC34 扫描、电池、LED、keymap 接入。

### 路线 3：nRF5 SDK 复刻 Xorlink 风格

用途：最大概率贴近 Xorlink 的技术选型。

优点：

- Nordic Secure DFU、SoftDevice、timeslot、ESB/proprietary radio 资料成熟。
- 更接近 Xorlink 更新方式。
- 对 BLE + proprietary radio 共存更容易参考 Nordic 老资料。

缺点：

- 和现有 ZMK 代码复用度低。
- EC34 电容扫描和 keymap 要重新写。
- 构建和维护体验不如 ZMK。

适用判断：

- 如果目标是真正 Unifying/Bolt 接收器兼容，而不是普通蓝牙键盘，这是更现实的主线。

### 路线 4：Zephyr/ZMK 内嵌 Unifying radio

用途：保留现有 ZMK 生态。

优点：

- 可复用现有 EC34 扫描/keymap/电池/LED。

缺点：

- ZMK 主要假设 BLE/USB HID，不是 Logitech proprietary radio device。
- 在 Zephyr 中同时跑 BLE controller 与 ESB/proprietary radio 的复杂度高。
- 需要处理调度、radio timeslot、功耗、连接切换。
- 更像长期研发，不像快速验证。

暂不建议作为第一主线。

## 最关键的未知点

1. Xorlink 的 Bolt 设备侧 discovery 广播/服务具体是什么。
2. Bolt receiver 是否接受 Craft 这种非 Bolt 官方产品身份，还是 Xorlink 实际在 Bolt 模式下使用了类似 `B363/B366` 的 Bolt 设备身份。
3. Options 识别 Craft 需要实现多少 HID++ 4.5 特性。
4. Unifying receiver 新固件对自制设备端的限制有多强。
5. nRF52840 上 radio + BLE 的共存策略，是用 nRF5 SoftDevice timeslot，还是完全分模式切换。

## 下一轮验证建议

### 1. 用 Solaar 或 Logi Web Connect 验证现有接收器

如果手头有 Bolt receiver / Unifying receiver：

- Windows 下可用 Logi Web Connect 观察能否发现设备。
- Linux 下 Solaar 更适合看 receiver kind、paired device、WPID、错误。

重点记录：

- Bolt receiver PID 是否为 `C548`。
- Unifying receiver PID 是否为 `C52B` 或 `C532`。
- 配对失败原因是 no device、device not supported、timeout 还是 passkey fail。

### 2. 找一把真正 Craft 或 MX Keys 做对照

如果能借到真 Logitech 键盘：

- 用 Solaar dump feature set。
- 记录 WPID/BTID/model ID/device name。
- 观察 Options 识别和功能查询。
- 只做公开层 GATT/HID++ 对比，不提取密钥、不复制 bond。

对 Bolt 方向优先找：

- MX Keys for Business，参考 ID `B363`
- MX Mechanical，参考 ID `B366`
- 其他明确标注 Logi Bolt 的键盘

对 Options/Craft 方向再找：

- Craft Advanced Keyboard，参考 WPID `4066` / BTID `B350`

### 3. 做 Unifying 最小设备端原型

先不接 EC34：

- 用 nRF52840 dongle 或 E73 dev board 做一个“最小键盘设备端”。
- 目标 receiver 用 C-U0007/C-U0008/C-U0012。
- 先以简单设备身份完成配对和一个按键输入。
- 成功后再改成 Craft WPID/feature set。

### 4. 成功后再回填 EC34

只有 receiver 侧连通后，才值得接入：

- EC34 电容扫描
- keymap
- battery ADC
- BT LED
- DFU 按键入口

这和你的判断一致：键值设定和引脚设置确实不是当前难点。

## 当前最可行结论

可行性排序：

1. Unifying-first 独立 nRF52840 原型：可行性最高，公开资料最多。
2. nRF5 SDK + SoftDevice + Secure DFU 复刻 Xorlink 架构：长期最像正确路线。
3. Bolt receiver 直接兼容：可行但未知点最大，需要实机抓 discovery/配对公开层行为。
4. 在现有 ZMK 上直接实现完整 Bolt/Unifying：短期不现实。

最终建议：

先把目标拆成“receiver 连通”而不是“EC34 固件”。第一阶段只做 Unifying-compatible nRF52840 原型，复刻公开资料里的设备端配对、HID++ keep-alive 和最小键盘报告。只要这个能和 Unifying receiver 完成配对并在系统里出现 Logitech 键盘，后续 Craft 身份、Options 识别、Bolt support 才有继续推进的基础。
