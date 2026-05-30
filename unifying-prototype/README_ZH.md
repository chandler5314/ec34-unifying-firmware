# EC34 Unifying 设备端原型

目标：让 E73/nRF52840 作为一个可以合法配对到用户自有 Logitech Unifying receiver 的键盘设备。

当前阶段是协议核心骨架，不直接操作 receiver，不包含强制配对、密钥嗅探或注入其他设备这类安全研究功能。

## 设计边界

- 主线目标是“接收器进入正常配对模式后，EC34 作为新键盘完成配对”。
- 初始 profile 采用简单键盘身份，便于排错；后续再评估是否切换到更复杂的 Logitech 键盘身份。
- radio 层暂时抽象为 `ec34u_radio_ops`，后面可以接：
  - nRF5 SDK proprietary radio / ESB 风格实现
  - Zephyr ESB / direct RADIO 实现
- EC34 电容扫描、keymap、电池、LED 暂不接入；优联链路跑通后再移植。

## 当前文件

- `include/ec34u_unifying.h`
  - 协议常量、profile、radio 抽象、公开 API。
- `src/ec34u_unifying.c`
  - 配对帧构建、checksum、keep-alive、普通键盘报告。
- `src/ec34u_unifying_crypto.c`
  - 加密键盘帧接口占位。后续只用于合法配对产生的链路密钥。
- `docs/IMPLEMENTATION_NOTES_ZH.md`
  - 当前实现范围和后续接 radio 的计划。

## 下一步

1. 用当前 C52B receiver 做主机侧 HID++ 只读探测。
2. 选择 radio 技术栈：
   - 如果优先复刻 Xorlink 风格：nRF5 SDK。
   - 如果优先复用现有 ZMK/Zephyr 环境：Zephyr ESB 或 direct RADIO。
3. 在独立 dev build 上实现 `ec34u_radio_ops`。
4. receiver 进入配对模式后，尝试发送配对阶段 1/2/3 帧并读取响应。
5. 只有合法配对成功后，再启用加密键盘报告和 EC34 按键扫描。
