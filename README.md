# AI智能管家——基于ESP32-S3与云端大模型的智能家居系统

> **参赛项目**：2026全国大学生物联网设计竞赛（乐鑫信息科技命题）
> **核心主控**：ESP32-S3-DevKitC-1-N16R8
> **开发框架**：ESP-IDF v5.5.4 + FreeRTOS

---

## 一、项目概述

本项目是一套**完全独立于PC/手机**的AIoT智能家居系统。ESP32-S3通过Wi-Fi直连互联网，集成多源环境传感器、BLE心率监测、语音交互、本地规则引擎、OLED显示和轻量Web服务器，实现环境监测、人体感知、大模型双向交互、自动联动四大核心功能。

### 核心创新点

1. **大模型融合决策**：ESP32直接通过HTTPS调用火山引擎豆包API（ASR+LLM+TTS），实现自然语言语音控制设备，定时上传感知数据获取个性化建议。
2. **光闹钟唤醒**：WS2812B LED灯带30分钟渐进亮度自然唤醒，支持纯灯光/灯光+蜂鸣器两种模式。
3. **心率异常预警**：实时接收智能手环心率数据，异常时本地声光报警+Server酱微信推送双重告警。
4. **完全离线可用**：断网时本地规则引擎、自动联动、异常报警功能正常运行。

---

## 二、硬件架构

### 2.1 核心主控

| 器件 | 型号 | 规格 |
|------|------|------|
| 主控开发板 | ESP32-S3-DevKitC-1-N16R8 | Xtensa LX7双核@240MHz, 16MB Flash, 8MB PSRAM, Wi-Fi+BLE 5.0 |

### 2.2 传感器（感知层）

| 传感器 | 型号 | 接口 | GPIO | 功能 |
|--------|------|------|------|------|
| 温湿度传感器 | DHT22 | 单总线 | GPIO4 | 温度(-40~80℃)、湿度(0-100%RH) |
| 光照传感器 | BH1750 | I2C | SDA=GPIO5, SCL=GPIO6 | 光照强度(1-65535 lux) |
| 人体红外传感器 | HC-SR501 | 数字输入 | GPIO7 | 人体存在检测(3-7米) |
| 烟雾传感器 | MQ-2 | ADC模拟输入 | GPIO8 (ADC1_CH7) | 烟雾/可燃气体浓度 |
| 数字麦克风 | INMP441 | I2S | BCLK=GPIO10, WS=GPIO11, SD=GPIO12 | 语音采集，硬件降噪 |
| 智能手环 | 华为/小米/荣耀等 | BLE 5.0 | — | 心率监测(0x180D服务) |

### 2.3 执行器

| 执行器 | 接口 | GPIO | 功能 |
|--------|------|------|------|
| 直流电机+风扇叶 | PWM | PWMA=GPIO13 | 卧室风扇调速 |
| 电机驱动 | 数字输出 | AIN1=GPIO16, AIN2=GPIO17 | 电机正反转控制 |
| 继电器-换气扇 | 数字输出 | GPIO18 | 卫生间换气扇通断 |
| 继电器-加湿器 | 数字输出 | GPIO9 | 卧室加湿器通断 |
| WS2812B LED灯带 | 单总线 | GPIO14 | 灯光控制/光闹钟 |
| 有源蜂鸣器 | 数字输出 | GPIO15 | 异常报警/闹钟提醒 |
| EC11旋转编码器 | 数字输入 | CLK=GPIO20, DT=GPIO21, SW=GPIO47 | 手动调速与确认 |

### 2.4 显示与交互

| 器件 | 接口 | GPIO/地址 | 功能 |
|------|------|-----------|------|
| 0.96寸OLED | I2C | SDA=GPIO5, SCL=GPIO6, 地址0x3C | 本地实时数据显示 |
| 按键1-语音唤醒 | 数字输入 | GPIO35 | 语音唤醒/模式切换 |
| 按键2-报警测试 | 数字输入 | GPIO36 | 手动触发报警测试 |
| 按键3-页面切换 | 数字输入 | GPIO37 | 切换OLED显示页面 |
| 按键4-紧急停止 | 数字输入 | GPIO38 | 系统紧急停止 |

---

## 三、软件架构

### 3.1 项目目录结构

```
ai_smart_home/
├── main/
│   ├── main.c              # 主程序入口
│   └── CMakeLists.txt      # main组件构建配置
│
├── components/             # ESP-IDF组件（模块化驱动）
│   ├── sensors/            # 传感器驱动
│   │   ├── dht22/          # DHT22温湿度传感器
│   │   ├── bh1750/         # BH1750光照传感器
│   │   ├── hcsr501/        # HC-SR501人体红外
│   │   └── mq2/            # MQ-2烟雾传感器
│   ├── ble/                # BLE心率接收
│   ├── audio/              # 语音采集与VAD
│   ├── display/            # OLED显示
│   │   ├── ssd1306/        # SSD1306驱动
│   │   └── ui/             # UI界面
│   ├── web/                # 轻量Web服务器
│   │   ├── server/         # HTTP服务器核心
│   │   ├── api/            # REST API路由
│   │   └── static/         # 静态页面（HTML/CSS/JS）
│   ├── actuator/           # 执行器驱动
│   │   ├── motor/          # TB6612FNG电机驱动
│   │   ├── relay/          # 继电器控制
│   │   ├── led/            # WS2812B LED灯带
│   │   ├── buzzer/         # 有源蜂鸣器
│   │   └── encoder/        # EC11旋转编码器
│   ├── notify/             # 微信消息推送（Server酱）
│   ├── llm/                # 大模型HTTP交互
│   │   ├── asr/            # 语音识别
│   │   ├── chat/           # 大模型对话
│   │   └── tts/            # 语音合成
│   ├── storage/            # 数据存储
│   │   └── sqlite/         # SQLite数据库
│   ├── rules/              # 本地规则引擎
│   │   ├── state/          # 状态判断
│   │   └── automation/     # 自动联动逻辑
│   └── esp32-idf-sqlite3/  # SQLite 库组件（已适配 ESP-IDF 5.x）
│
├── hardware/               # 硬件资料
├── docs/                   # 技术文档
├── CMakeLists.txt          # 项目根配置
├── sdkconfig.defaults      # SDK 默认配置
├── partitions.csv          # 自定义分区表
└── README.md               # 本文件
```

---

## 四、编译与烧录

### 4.1 环境要求

- **ESP-IDF**：v5.5.4
- **开发板**：ESP32-S3-DevKitC-1-N16R8（16MB Flash + 8MB PSRAM）
- **Python**：3.8+
- **操作系统**：Windows 10/11 或 Ubuntu 20.04+

### 4.2 首次编译步骤

```bash
# 1. 进入项目目录
cd ai_smart_home

# 2. 设置ESP-IDF环境（Windows）
%userprofile%\esp\esp-idf\export.bat

# 3. 配置项目（重要！）
idf.py menuconfig
#   - Component config -> Bluetooth -> 启用 Bluetooth
#   - Component config -> PSRAM -> 启用 SPI RAM
#   - Partition Table -> 选择 Custom partition table CSV -> partitions.csv

# 4. 编译
idf.py build

# 5. 烧录（替换COM口）
idf.py -p COM3 flash

# 6. 查看串口日志
idf.py -p COM3 monitor
```

### 4.3 sdkconfig.defaults 关键配置

以下选项必须在 `sdkconfig.defaults` 中启用：

```
CONFIG_BT_ENABLED=y
CONFIG_BT_BLE_ENABLED=y
CONFIG_SPIRAM=y
CONFIG_SPIRAM_USE_CAPS_ALLOC=y
CONFIG_ESP_HTTP_CLIENT_ENABLE_HTTPS=y
CONFIG_MBEDTLS_TLS_CLIENT_ONLY=y

# SQLite 相关配置
CONFIG_PARTITION_TABLE_CUSTOM=y
CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="partitions.csv"
CONFIG_PARTITION_TABLE_FILENAME="partitions.csv"
CONFIG_ESP_MAIN_TASK_STACK_SIZE=8192       # 主任务栈增大（SQLite需要）
CONFIG_FREERTOS_TIMER_TASK_STACK_DEPTH=4096 # 定时器任务栈增大
```

---

## 五、配置说明

### 5.1 Wi-Fi配置

修改 `main/main.c` 中的宏定义：

```c
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASS "YOUR_WIFI_PASSWORD"
```

### 5.2 大模型API密钥

修改各组件中的占位符为真实密钥：

- `components/llm/chat/chat.c`：`#define DOUBAO_API_KEY "your_api_key_here"`
- `components/llm/asr/asr.c`：`#define ASR_ACCESS_TOKEN "your_token_here"`
- `components/llm/tts/tts.c`：`#define TTS_ACCESS_TOKEN "your_token_here"`
- `components/notify/notify.c`：`#define WECHAT_SENDKEY "your_sendkey_here"`

### 5.3 获取火山引擎豆包API密钥

1. 访问 [火山引擎官网](https://www.volcengine.com/)
2. 注册账号并实名认证
3. 进入「模型广场」→「豆包大模型」
4. 创建API Key，复制到代码中
5. **乐鑫专项**：通过乐鑫官方渠道申请200万免费Token额度

---

## 六、使用说明

### 6.1 上电启动流程

1. 接通5V 2A电源，ESP32-S3启动
2. 串口输出日志：`AI Smart Home Starting...`
3. 连接Wi-Fi，获取IP地址（日志输出：`Got IP: 192.168.x.x`）
4. 初始化所有传感器和执行器
5. OLED显示数据页，Web服务器启动
6. 打开手机浏览器，访问 `http://192.168.x.x` 查看监控面板

### 6.2 语音控制

1. 对着INMP441麦克风说出指令（如"打开卧室灯"、"把风扇关掉"）
2. 系统蜂鸣器短鸣提示检测到语音
3. ASR识别文字 → 大模型解析 → Function Calling执行设备控制 → TTS语音播报结果

### 6.3 Web远程控制

1. 手机/电脑连接同一Wi-Fi
2. 浏览器访问ESP32的IP地址
3. 查看实时传感器数值和数据曲线
4. 点击按钮控制设备（开灯、调速、加湿等）
5. 设置闹钟时间和唤醒模式

### 6.4 离线运行

当Wi-Fi断开时：
- 本地规则引擎继续运行（温度联动风扇、湿度联动加湿器、睡眠关灯等）
- 传感器数据继续采集并存储到SQLite
- OLED正常显示
- **无法使用**：大模型语音交互、微信推送、Web远程访问

---

## 七、开发团队与分工

| 成员 | 职责 | 负责模块 |
|------|------|---------|
| 成员A | 硬件与嵌入式核心 | sensors/、ble/、audio/、actuator/、rules/ |
| 成员B | 云端交互与显示 | display/、web/、llm/、storage/、notify/ |

---

## 八、许可证

本项目为2026全国大学生物联网设计竞赛参赛作品，代码仅供学习交流使用。

---

> **最后更新**：2026-06-07
> **项目仓库**：ai_smart_home/
