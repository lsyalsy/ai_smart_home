# AI 智能管家——基于 ESP32-S3 的智能家居系统

> **参赛项目**：2026 全国大学生物联网设计竞赛
> **核心主控**：ESP32-S3-DevKitC-1-N16R8
> **开发框架**：ESP-IDF v5.5.4 + FreeRTOS

---

## 一、项目概述

本项目是一套基于 ESP32-S3 的 AIoT 智能家居系统，集成多源环境传感器、BLE 心率监测、本地显示、执行器联动等功能，目标是实现环境监测、人体感知、自动控制与多端交互。

### 已实现的硬件功能

1. **TFT 本地显示**：1.8 寸 ST7735 彩屏显示数据页、建议页、状态页、闹钟页。
2. **环境感知**：DHT22 温湿度、BH1750 光照、HC-SR501 人体红外、MQ-2 烟雾传感器。
3. **BLE 心率接收**：ESP32-S3 内置 BLE 5.0 连接智能手环，读取心率服务 0x180D。
4. **执行器控制**：LED、蜂鸣器、风扇电机、加湿器继电器。
5. **人机交互**：EC11 旋转编码器（旋转切页/调时、按键确认）+ GPIO37 备用按键。

### 待实现功能

- Wi-Fi 联网、轻量 Web 服务器
- 语音交互（INMP441 + ASR/LLM/TTS）
- 本地规则引擎自动联动
- Server 酱微信推送
- SQLite 数据存储与历史曲线
- WS2812B 光闹钟

---

## 二、硬件架构

### 2.1 核心主控

| 器件 | 型号 | 规格 |
|------|------|------|
| 主控开发板 | ESP32-S3-DevKitC-1-N16R8 | Xtensa LX7 双核 @240MHz，16MB Flash，8MB PSRAM，Wi-Fi + BLE 5.0 |

### 2.2 显示模块

| 器件 | 型号 | 接口 | GPIO | 功能 |
|------|------|------|------|------|
| TFT 显示屏 | ST7735 | 软件 SPI | SCK=GPIO1, MOSI=GPIO7, CS=GPIO20, DC=GPIO21, RST=GPIO47, BL=GPIO45 | 1.8 寸 128×160 彩屏本地显示 |

> 说明：原设计使用 0.96 寸 SSD1306 OLED，已替换为 1.8 寸 TFT 彩屏。

### 2.3 传感器（感知层）

| 传感器 | 接口 | GPIO | 功能 |
|--------|------|------|------|
| DHT22 温湿度 | 单总线 | GPIO3 | 温度、湿度 |
| BH1750 光照 | I2C | SDA=GPIO5, SCL=GPIO6 | 光照强度（lux） |
| HC-SR501 人体红外 | 数字输入 | GPIO4 | 人体存在检测 |
| MQ-2 烟雾 | ADC | GPIO2（ADC1_CH1） | 烟雾/可燃气体浓度 |
| 智能手环 | BLE 5.0 | 无 GPIO | 心率监测（0x180D / 0x2A37） |

### 2.4 执行器

| 执行器 | GPIO | 功能 |
|--------|------|------|
| LED 灯 | GPIO15 | 灯光开关 |
| 有源蜂鸣器 | GPIO8 | 报警/提示音 |
| 风扇电机 | GPIO16 | 风扇开关 |
| 加湿器继电器 | GPIO17 | 加湿器开关 |

### 2.5 人机交互

| 器件 | GPIO | 功能 |
|------|------|------|
| EC11 旋转编码器 CLK | GPIO38 | 旋转 A 相 |
| EC11 旋转编码器 DT | GPIO39 | 旋转 B 相 |
| EC11 旋转编码器 SW | GPIO40 | 按键确认 |
| 备用页面按键 | GPIO37 | 页面切换 |

### 2.6 保留 / 禁用引脚

| 功能 | 引脚 | 说明 |
|------|------|------|
| USB 调试 / 下载 | GPIO43(TX) / GPIO44(RX) | 板载 USB 转串口 |
| Strapping 引脚 | GPIO0 | 低电平进入下载模式 |
| 板载 RGB LED | GPIO48 | 已被开发板占用 |
| 内部 Flash / PSRAM | GPIO33 / GPIO34 / GPIO35 | 不可接外设，否则系统崩溃 |

更详细的接线表见 [`docs/pinout.md`](docs/pinout.md)。

---

## 三、软件架构

### 3.1 项目目录结构

```
ai_smart_home/
├── main/
│   ├── main.c              # 主程序入口（硬件综合测试）
│   └── CMakeLists.txt      # main 组件构建配置
│
├── components/             # ESP-IDF 组件（模块化驱动）
│   ├── st7735/             # TFT 屏幕驱动（软件 SPI）
│   ├── ui/                 # TFT 页面渲染（4 页面 + 局部刷新）
│   ├── input/
│   │   └── ec11/           # EC11 旋转编码器驱动
│   ├── sensors/
│   │   ├── dht22/          # DHT22 温湿度
│   │   ├── bh1750/         # BH1750 光照
│   │   ├── hcsr501/        # HC-SR501 人体红外
│   │   └── mq2/            # MQ-2 烟雾（ADC）
│   ├── ble/
│   │   └── ble_hr.c        # BLE GATT Client 心率接收
│   ├── actuator/
│   │   ├── led/            # LED 灯
│   │   ├── buzzer/         # 蜂鸣器
│   │   ├── motor/          # 风扇电机
│   │   └── relay/          # 加湿器继电器
│   └── esp32-idf-sqlite3/  # SQLite 库组件（预留）
│
├── docs/
│   └── pinout.md           # 硬件引脚对照表
├── build/                  # 构建输出目录
├── CMakeLists.txt          # 项目根配置
├── sdkconfig.defaults      # SDK 默认配置
└── README.md               # 本文件
```

### 3.2 刷新策略

为避免数据更新时整屏闪烁，UI 采用分级刷新：

- `ui_render_page(page, state)`：切换页面时调用，清屏并重绘标题栏 + 内容。
- `ui_update_page(state)`：同页数据变化时调用，只重绘右侧数值/状态区域。

---

## 四、编译与烧录

### 4.1 环境要求

- **ESP-IDF**：v5.5.4（项目原设计版本；本机已验证 v5.5.2 可编译）
- **开发板**：ESP32-S3-DevKitC-1-N16R8
- **Python**：3.11（通过 ESP-IDF 安装器部署）
- **操作系统**：Windows 10/11

### 4.2 首次编译步骤

```powershell
# 进入项目目录
cd ai_smart_home

# 设置 ESP-IDF 环境（根据实际安装路径调整）
C:\esp\v5.5.4\esp-idf\export.ps1

# 编译
idf.py build

# 烧录（替换为实际 COM 口）
idf.py -p COM3 flash

# 查看串口日志
idf.py -p COM3 monitor
```

> 本机当前实际安装的是 v5.5.2（路径 `C:\esp\v5.5.2\esp-idf`），可正常编译；若后续重新安装 v5.5.4，请对应修改 export 脚本路径。

### 4.3 关键 SDK 配置

`sdkconfig.defaults` 中已启用：

```
CONFIG_BT_ENABLED=y
CONFIG_BT_NIMBLE_ENABLED=y
CONFIG_BT_BLUEDROID_ENABLED=n
CONFIG_BT_NIMBLE_ROLE_CENTRAL=y
CONFIG_BT_NIMBLE_ROLE_OBSERVER=y
CONFIG_BT_NIMBLE_GATT_CLIENT=y
```

---

## 五、使用说明

### 5.1 上电启动流程

1. 接通 5V 电源，ESP32-S3 启动。
2. 串口输出日志：`AI Smart Home hardware test starting...`
3. TFT 显示第一页（数据页）。
4. 传感器每 2 秒刷新一次，执行器每 5 秒自动切换一次方便测试。

### 5.2 页面切换

- **数据页**：温度、湿度、光照、人体、烟雾、心率。
- **建议页**：大模型建议文本（当前为占位文本）。
- **状态页**：LED、风扇、加湿器、报警状态。
- **闹钟页**：闹钟时间、唤醒模式、开关状态。

旋转编码器顺时针/逆时针可切换页面；在闹钟页面旋转可调整分钟。

### 5.3 心率接收

1. 确保智能手环未连接手机/电脑，并开启心率广播。
2. 部分品牌手环需在配套 APP 中开启“第三方接入”或“蓝牙广播”。
3. 系统会自动扫描并连接广播 0x180D 服务的设备，串口输出心率值。

### 5.4 当前测试模式

为便于硬件逐一验证，`main.c` 中默认开启执行器自动轮询：

```c
#define ACTUATOR_TEST_MS 5000   /* 每 5 秒自动切换 LED/电机/继电器/蜂鸣器 */
```

正式运行时可将该逻辑替换为规则引擎或 Web/语音控制。

---

## 六、开发计划

| 阶段 | 内容 | 状态 |
|------|------|------|
| 硬件驱动 | TFT、传感器、执行器、编码器、BLE 心率 | 已完成 |
| 本地 UI | 4 页面显示 + 局部刷新 | 已完成 |
| 规则引擎 | 温度/湿度/烟雾/人体联动 | 待实现 |
| 联网模块 | Wi-Fi、Web 服务器、NTP | 待实现 |
| 语音交互 | INMP441、ASR、LLM、TTS | 待实现 |
| 数据存储 | SQLite 历史记录 | 待实现 |
| 通知推送 | Server 酱微信推送 | 待实现 |

---

## 七、许可证

本项目为 2026 全国大学生物联网设计竞赛参赛作品，代码仅供学习交流使用。

---

> **最后更新**：2026-06-24
