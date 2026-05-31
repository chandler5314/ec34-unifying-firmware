# EC34 / Tako34 模拟 Logi Bolt 可行性研究

生成时间：2026-05-30  
工作目录：`D:\Inuitive\EC34\EC34bolt`

## 结论摘要

EC34 使用的 E73/nRF52840 从硬件能力上可以做 BLE HID 键盘，也具备实现 BLE Secure Connections / Passkey / Security Mode 1 Level 4 这类安全配对能力的基础。问题在于：Logi Bolt 并不是“任何 BLE HID 键盘都能连的接收器”。罗技官方说明 Bolt 是基于 BLE 的闭环系统，Logi Bolt USB 接收器只连接 Logi Bolt 产品；Unifying 又是另一套罗技 2.4GHz 专有协议，不能和 Bolt 互配。

所以，如果目标是“让 EC34 在电脑端像普通蓝牙键盘一样可用”，ZMK 已经完成。如果目标是“让 EC34 直接配对罗技 Bolt USB 接收器，并被 Logi Web Connect/Options+ 当作 Logi Bolt 键盘”，这不是简单改 ZMK 设备名、HID descriptor 或打开一个 Kconfig 就能保证完成的事情。它很可能需要 Logitech 设备识别、专有服务/元数据、配对策略和接收器侧白名单/产品判定的兼容实现。

Xavier1989 的 HHKB DMC 公开文档确实声称固件 V1.5.0 起支持 Logi Bolt，V1.4.0 起支持 Unifying，但公开 GitHub 里没有找到对应 nRF52840/E73 的 Bolt/Unifying 实现源码。下载到的固件包是 Nordic DFU 的 `.bin/.dat` 裸固件，不是可直接移植的 ZMK 模块。

## 已收集资料

### 本地资料

- Xavier1989 文档仓库：
  - `D:\Inuitive\EC34\EC34bolt\repos\gitbook_hhkb\gitbook_hhkb-master`
- Xavier1989 旧代码仓库：
  - `D:\Inuitive\EC34\EC34bolt\repos\USB_BLE_Keyboard\USB_BLE_Keyboard-master`
  - 该仓库是 STM32 + Dialog BLE 的早期双模键盘工程，不是当前 E73/nRF52840 的 Bolt 实现源码。
- HHKB DMC V1.7.0 固件包：
  - `D:\Inuitive\EC34\EC34bolt\firmware\hhkb_pro2_boot_app_dfu_package_V1.7.0.zip`
  - `D:\Inuitive\EC34\EC34bolt\firmware\hhkb_bt_boot_app_dfu_package_V1.7.0.zip`
  - `D:\Inuitive\EC34\EC34bolt\firmware\hhkb_hybrid_boot_app_dfu_package_V1.7.0.zip`
- 解包后每个固件包都有 application 和 bootloader：
  - `*_app.bin` 约 174-176 KB
  - `*_boot.bin` 约 45 KB
  - `*_app.dat` / `*_boot.dat`
  - `manifest.json`

### 公开网页资料

- Logitech 官方：Bolt 和 Unifying 区别  
  https://support.logi.com/hc/en-ch/articles/1500012483162-What-is-the-difference-between-Bolt-and-Unifying-receivers
- Logitech 官方：Logi Bolt 技术页  
  https://www.logitech.com/en-us/business/work-setups/logi-bolt-wireless-technology.html
- Logitech 官方：Logi Bolt 白皮书  
  https://www.logitech.com/content/dam/logitech/en/business/pdf/logi-bolt-white-paper.pdf
- Logitech 官方：Logi Bolt / Unifying 键盘配对流程  
  https://support.logi.com/hc/en-sg/articles/4404722646167-How-to-pair-and-unpair-a-Logi-Bolt-Unifying-keyboard-using-the-Logi-Bolt-app-Logi-Web-Connect
- ZMK 官方蓝牙文档  
  https://zmk.dev/docs/features/bluetooth
- ZMK 官方蓝牙配置  
  https://zmk.dev/docs/config/bluetooth
- Zephyr 官方 Secure Connections Only 示例  
  https://docs.zephyrproject.org/latest/samples/bluetooth/peripheral_sc_only/README.html

## Xavier1989 / HHKB DMC 能证明什么

公开文档中有几条很重要：

- `logitech_bolt.md` 写明：V1.5.0 起 HHKB DMC 支持 Bolt，兼容 Logi USB Bolt Receiver。
- 配对方式是浏览器访问 `https://logiwebconnect.com`，网页识别接收器后添加设备。
- 首次 Bolt 配对和蓝牙类似，需要输入 6 位配对码。
- 连接后页面显示“已连接 Logi Bolt”。
- `logitech_unifying.md` 写明：V1.4.0 起支持 2.4G Unifying，兼容 C-U0007、C-U0008、C-U0012 等接收器。
- `mutil_bond.md` 写明：蓝牙、Bolt、Unifying 都支持多设备绑定和切换。

这些能证明 HHKB DMC 的二进制固件在用户层面做到了 Bolt/Unifying 连接。但它没有说明协议细节，也没有公开可移植代码。

## 固件包反编译价值判断

下载到的 V1.7.0 固件是 Nordic DFU 包，结构是裸 `.bin` + `.dat`，不是带符号的 ELF，也不是 ZMK 常见的 UF2 成品。对 app/boot bin 做字符串扫描，基本没有出现 `bolt`、`logi`、`unifying`、`keyboard`、`pair` 等可读实现线索。

理论上可以用 Ghidra/IDA 对 ARM Cortex-M 裸 bin 做反汇编，但现实收益很低：

- 没有符号表、函数名、类型信息。
- 需要先确认 flash 起始地址、SoftDevice/bootloader/app 布局和中断向量。
- 即使能还原部分逻辑，也很难可靠移植到 ZMK/Zephyr。
- 可能触及第三方固件版权、商业秘密或安全边界。

因此当前阶段不建议走“反编译移植”路线。

## Bolt、普通 BLE、Unifying 的边界

### 普通 ZMK BLE

ZMK 官方说明它通过 BLE 连接主机，配对后保存 bond 信息；ZMK 也使用 BLE Secure Connections，禁用 Legacy pairing。ZMK 配置里还有 `CONFIG_ZMK_BLE_PASSKEY_ENTRY`，可以启用配对时输入 passkey。

当前 EC34 配置里已有：

```conf
CONFIG_ZMK_BLE=y
CONFIG_ZMK_USB=y
CONFIG_ZMK_BATTERY_REPORTING=y
```

也就是说，EC34 现在已经是一个标准 BLE HID 键盘。

### Logi Bolt

罗技官方资料显示：

- Bolt 基于 Bluetooth Low Energy 5.0 或更高。
- 连接 Logi Bolt USB receiver 时，键盘/鼠标使用 Security Mode 1 Level 4，也就是 Authenticated LE Secure Connections。
- 键盘与 Bolt receiver 配对时使用 6 位 passkey。
- Bolt receiver 最多可配对 6 个 Logi Bolt 设备。
- 官方明确说：虽然 Bolt 基于 Bluetooth，但它是端到端闭环系统，Logi Bolt receiver 只连接 Logi Bolt 产品，不能和非 Logi Bolt 设备配对。

这说明：nRF52840 的 BLE 能力是必要条件，但不是充分条件。

### Unifying

罗技官方说明 Unifying 是罗技开发的 proprietary 2.4GHz RF 协议，不是 BLE。Bolt 和 Unifying “不说同一种语言”，不能互配。

因此 Unifying 支持不应该被理解为“打开 ZMK BLE 的另一个选项”。它更接近另外写一套 2.4GHz radio 协议栈，并且要处理接收器配对、设备身份、加密、HID 报告和无线重连。这个方向和 ZMK 当前 BLE 主栈的耦合很重，风险明显高于 Bolt probe。

## 对 EC34/ZMK 的可行性判断

### 1. 直接让现有 ZMK 固件成为 Logi Bolt 设备

可行性：低到中，未知项很多。

如果 Bolt receiver 只检查 BLE HID、安全等级和 passkey，那么 Zephyr/ZMK 理论上能逐步靠近：

- 开启 passkey 输入。
- 研究是否能启用 SC-only / Security Level 4。
- 调整 HID descriptor、Appearance、Device Information Service。
- 测试 Logi Web Connect 是否能发现设备。

但罗技官方“receiver 只连接 Logi Bolt 产品”的描述说明接收器大概率有额外产品识别条件。这个条件目前没有公开文档，Xavier 的实现也没有源码。

### 2. 在 ZMK 上做一个“Bolt 探测版”固件

可行性：中，适合作为下一步实验。

这个版本不应假装已经支持 Bolt，只做非侵入式验证：

- 新建独立分支，不动现在可用的 EC34 固件。
- 保留现有按键、电池、蓝牙 LED。
- 启用 `CONFIG_ZMK_BLE_PASSKEY_ENTRY=y` 或 ZMK 的实验安全配置。
- 视情况尝试 Zephyr 的 SC-only 配置，但必须防止普通蓝牙连接被全部破坏。
- 使用 Logi Web Connect + 自有 Bolt receiver 测试它是否能在“添加设备”时看到 EC34。

如果 Logi Web Connect 能看到 EC34，但连接失败，再继续看是 passkey、安全等级、GATT 服务还是 HID descriptor 问题。  
如果完全看不到，说明可能卡在 receiver 的 Logi 产品筛选或特定广播/服务识别上。

### 3. 克隆或伪装罗技设备身份

不建议。

如果实现路径需要复制罗技设备身份、私有标识、密钥、白名单行为或绕过 receiver 的产品检查，这会进入法律和安全风险区。我可以继续做高层分析、合法互操作实验设计、ZMK/Zephyr 侧安全配对能力验证，但不提供绕过认证、提取密钥或冒充特定商业设备的操作步骤。

### 4. 做一个自有 nRF52840 USB 接收器

可行性：高，工程上更干净。

这条路不是 Logi Bolt，但能获得“一个 USB 小接收器 + 键盘无线连接 + 电脑端免蓝牙”的体验：

- 用 nRF52840 USB dongle 做接收器。
- 接收器枚举成 USB HID 键盘。
- EC34 和 dongle 之间用 BLE 或自定义 2.4GHz 链路通信。
- 可以完全开源、可维护、可调试，不依赖 Logitech receiver。

如果最终目标是稳定、低延迟、少折腾，这条路线比真 Bolt 兼容更实际。

## 建议的下一步实验

### A. 最小验证：当前 EC34 + Bolt receiver

1. 插入自有 Logi Bolt receiver。
2. 用 Chrome/Edge 打开 `https://logiwebconnect.com`。
3. 选择接收器并进入 Add Device。
4. 让当前 EC34 进入蓝牙可配对状态。
5. 记录结果：
   - 是否出现 EC34 设备名？
   - 是否要求输入 6 位 passkey？
   - 是否显示 unsupported / not found / connection failed？
   - 是否在 receiver 侧完成绑定？

这个实验不改固件，能快速判断“普通 ZMK BLE HID 是否有机会被 Bolt receiver 扫到”。

### B. Probe 固件：ZMK 安全配对增强

如果 A 看不到或配对失败，可以做一个单独 probe 固件：

```conf
CONFIG_ZMK_BLE_PASSKEY_ENTRY=y
# 视测试结果再评估：
# CONFIG_ZMK_BLE_EXPERIMENTAL_SEC=y
# CONFIG_BT_SMP_SC_ONLY=y
```

注意：`CONFIG_BT_SMP_SC_ONLY` 可能影响普通主机蓝牙兼容性，不适合作为日常固件直接开启。

### C. BLE/GATT 对比

如果能借到一把真正的 Logi Bolt 键盘，可以只做合法的公开层对比：

- 广播包里有哪些标准 UUID。
- Appearance 是否是 keyboard。
- Device Information Service 的字段结构。
- HID over GATT 的 report map 差异。
- Battery Service 行为。
- 配对时是否强制 MITM/passkey/LESC。

不要提取或复制私钥、bond key、设备唯一身份，也不要试图绕过 receiver 的产品检查。

### D. 如果 probe 失败，转向自有 dongle

如果普通 ZMK BLE + passkey + SC-only 都无法被 Bolt receiver 接受，说明主要障碍不是 BLE 安全等级，而是 Logitech 专有兼容层。此时继续在 ZMK 里猜协议成本会很高，建议转向自有 nRF52840 USB dongle 方案。

## 粗略工作量评估

- 当前 ZMK BLE probe 固件：0.5-1 天。
- Logi Web Connect 实机验证和日志整理：0.5 天。
- 与真 Logi Bolt 键盘做 BLE/GATT 公开层对比：1-2 天。
- 如果只是修 ZMK passkey/security/HID descriptor：1-3 天。
- 如果需要实现 Logitech 私有兼容层：不可估，且有法律/安全风险。
- 自有 nRF52840 dongle：基础可用原型约 3-7 天，打磨低功耗/重连/多设备可能更久。

## 当前建议

先不要动已经完成的 EC34 日常固件。下一步最好做一个独立的 `bolt-probe` 分支，只验证三件事：

1. 当前普通 ZMK BLE 能不能被 Logi Web Connect 在 Bolt receiver 添加设备流程中看到。
2. 开启 ZMK passkey 后是否能通过 6 位 passkey 阶段。
3. 是否需要 SC-only / Level 4 才能继续。

如果这三步都过不去，说明 HHKB DMC 的实现很可能包含未公开的 Logitech 兼容层，不适合直接塞进 ZMK。届时更建议做自有 dongle，或者仅保留标准蓝牙连接。
