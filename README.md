# ESP32-S3 Smart Desktop IoT Terminal

> **ESP32-S3 + LVGL v9.5 + FreeRTOS + EMQX — 多功能桌面智能 IoT 终端**
>
> 温湿度/光照采集 · 和风天气 · NTP 网络时钟 · MQTT 远程控制 · 双分区 OTA 升级

基于 ESP32-S3 打造的全功能桌面物联网终端，采用 FreeRTOS 多任务统一架构，搭载 LVGL 流畅触控界面，集成环境感知、网络服务、云端互联与安全 OTA 能力，可直接用于原型验证与二次开发。

---

### GPIO 引脚分配

| 功能 | 引脚 | 协议 | 备注 |
|------|------|------|------|
| LCD MOSI | GPIO7 | SPI2 | |
| LCD SCLK | GPIO15 | SPI2 | |
| LCD CS | GPIO4 | SPI2 | |
| LCD DC | GPIO6 | — | 数据/命令切换 |
| LCD RST | GPIO5 | — | 硬件复位 |
| LCD BL | GPIO8 | PWM | 背光调光 |
| Touch SDA | GPIO10 | I2C_NUM_1 | |
| Touch SCL | GPIO9 | I2C_NUM_1 | |
| Touch INT | GPIO11 | — | 中断 |
| Touch RST | GPIO12 | — | 硬件复位 |
| SHT30 SDA | GPIO1 | I2C | 与 BH1750 共享 |
| SHT30 SCL | GPIO2 | I2C | 与 BH1750 共享 |
| BH1750 SDA | GPIO1 | I2C | 与 SHT30 共享 |
| BH1750 SCL | GPIO2 | I2C | 与 SHT30 共享 |
| 继电器 | GPIO14 | — | 高电平触发 |

---

## 🧱 硬件方案

### 核心配置
| 组件 | 规格参数 |
|------|----------|
| 主控 | ESP32-S3-WROOM-1 (16MB Flash / 8MB PSRAM) |
| 显示屏 | ILI9341 2.8" TFT 电容屏，240×320 分辨率 |
| 触控芯片 | FT6336 电容触控 |
| 环境传感器 | SHT30 温湿度 + BH1750 数字光照 |
| 执行器 | 高电平触发继电器模块 |

### 硬件干扰优化
针对 WiFi 射频干扰 SPI 总线导致的屏幕黑线闪烁问题，采用两项折中优化：
- 屏幕背光默认限制为 **25% 亮度**
- WiFi 发射功率限制为 **2dBm**，降低射频干扰强度

---

## ⚙️ 系统架构

### 总体架构
```
┌─────────────────────────────────────────────┐
│              ESP32-S3 (N16R8)               │
│                                             │
│  ┌─────────┐  ┌──────────┐  ┌───────────┐   │
│  │ GUI任务 │ │ 传感器任务 │   │  网络任务│    │
│  │ (LVGL)  │  │SHT30+BH  │  │MQTT/HTTP  │   │
│  └────┬────┘  └────┬─────┘  └─────┬─────┘   │
│       │            │              │         │
│  ┌────┴────────────┴──────────────┴─────┐   │
│  │         FreeRTOS 内核                │    │
│  │    队列 / 事件组 / 互斥锁 / 定时器     │   │
│  └──────────────────────────────────────┘   │
│       │            │         │         │    │
│   SPI总线       I2C总线    WiFi      GPIO    │
│    (LCD)    (触摸+传感器)  (内置)   (继电器)  │
└───────┬────────────┬─────────┬─────────┬────┘
        │            │         │         │
   ┌────┴────┐  ┌────┴────┐    │   ┌────┴────┐
   │2.8寸TFT │  │SHT30    │    │   │继电器    │
   │触摸屏    │  │BH1750   │  EMQX  │         │
   └─────────┘  └─────────┘  Broker└─────────┘
```

### Flash 分区表（16MB）
自定义 `partitions.csv`，支持双 OTA 分区与异常回滚机制：

|名字|类型| SubType | Offset | Size | 说明 |
|------|------|---------|--------|------|------|
| nvs | data | nvs | 0x9000 | 24KB (0x6000) | NVS 键值对存储 |
| otadata | data | ota | 0xF000 | 8KB (0x2000) | OTA 分区选择与回滚标记 |
| phy_init | data | phy | 0x11000 | 4KB (0x1000) | WiFi PHY 校准数据 |
| ota_0 | app | ota_0 | 0x20000 | 3MB (0x300000) | 运行分区 A |
| ota_1 | app | ota_1 | 0x320000 | 3MB (0x300000) | 运行分区 B |

> 双分区大小一致，OTA 写入非当前运行分区，重启后切换；剩余空间预留后续文件系统扩展。

对应 `platformio.ini` 配置：
```ini
board_build.partitions = partitions.csv
board_upload.flash_size = 16MB
```

### 任务间通信机制
```
Sensor_Task ──[sensor_gui_queue]──→ Gui_Task （更新界面显示）
Sensor_Task ──[sensor_net_queue]──→ Network_Task （MQTT 云端上报）
Gui_Task ────[ctrl_event]─────────→ OTA_Task （触控触发 OTA）
Network_Task ←──[EMQX Broker]────── （下行指令接收
- 传感器数据通过两个独立 `xQueueOverwrite`（长度=1）分发，消费者永远拿到最新值
- 全局事件组 `wifi_ev` 同步 WiFi / MQTT / OTA 状态

```

## ✨ 功能特性

### 基础功能
- **三页触控 UI**：基于 LVGL v9.5 构建主页 / 数据曲线 / 设置页，原生 Tab 切换
- **环境采集**：SHT30 高精度温湿度 + BH1750 光照，每秒采样刷新
- **网络服务**：WiFi STA 自动重连、NTP 东八区自动校时、和风天气 API 对接
- **本地控制**：触控界面手动切换继电器、调节背光亮度
- **持久化存储**：NVS 保存温湿度阈值、背光亮度，掉电不丢失
- **双分区 OTA**：支持 HTTP 固件升级，自带异常回滚保护机制

### MQTT 进阶特性（EMQX Broker）
对接 EMQX 消息服务器，完整支持 MQTT 核心进阶能力：
- **LWT 遗嘱消息**：设备异常掉线时，Broker 自动发布离线状态通知
- **QoS 分级策略**：传感器上报采用 QoS 0（高频低开销），控制指令采用 QoS 1（可靠确认）
- **Retained 消息**：传感器最新数据发布保留标记，新订阅端可立即获取最新值

### 可靠性设计
- WiFi 断线自动重连，MQTT 断连自动重连并恢复订阅
- OTA 固件 SHA256 校验，升级失败自动回滚至上一稳定版本
- 硬件干扰优化，缓解屏幕闪烁问题

---

## 📂 项目结构

```
.gitignore
CMakeLists.txt
platformio.ini
partitions.csv
include/
├── lv_conf.h
└── lv_port.h
src/
├── CMakeLists.txt
├── atomic.h
├── main.c                      # 程序入口，统一初始化硬件、网络、FreeRTOS任务
├── MY_I2C/                     # 通用I2C总线驱动封装
│   ├── MY_I2C.c
│   └── MY_I2C.h
├── NVS/                        # NVS读写封装（仅存储阈值、背光）
│   ├── NVS.c
│   └── NVS.h
├── FreeRTOS_Task/              # 四大任务统一创建、队列/事件组/互斥锁管理
│   ├── FreeRTOS_Task.c
│   └── FreeRTOS_Task.h
├── Device/                     # 所有外设硬件驱动
│   ├── BH1750/
│   │   ├── BH1750.c
│   │   └── BH1750.h
│   ├── SHT30/
│   │   ├── SHT30.c
│   │   └── SHT30.h
│   ├── Relay/
│   │   ├── Relay.c
│   │   └── Relay.h
│   └── LCD/
│       ├── LCD_Init.c          # ILI9341屏幕SPI初始化
│       ├── LCD_Init.h
│       ├── FT6336_Touch.c      # FT6336电容触摸驱动
│       └── FT6336_Touch.h
├── UI/                         # LVGL页面布局、绘图、数据更新逻辑
│   ├── UI_main.c
│   ├── UI_main.h
│   ├── UI_data.c
│   └── UI_data.h
└── IOT/                        # 所有网络相关业务模块
    ├── WIFI_Init/              # WiFi连接、自动重连
    │   ├── WIFI_Init.c
    │   └── WIFI_Init.h
    ├── Get_Weather_Time/
    │   ├── Get_Time/
    │   │   ├── Ntp_Time.c      # NTP网络校时
    │   │   └── Ntp_Time.h
    │   └── Get_Weather/
    │       ├── Weather_HTTPS.c # HTTPS请求和风天气接口
    │       ├── Weather_HTTPS.h
    │       ├── Weather_Parse.c # cJSON解析天气数据
    │       └── Weather_Parse.h
    ├── EMQX/                   # EMQX MQTT客户端、LWT/QoS/Retained实现
    │   ├── EMQX.c
    │   ├── EMQX.h
    │   ├── EMQX_dm.c
    │   └── EMQX_dm.h
    └── OTA/                    # HTTP双分区OTA升级、回滚逻辑
        ├── OTA.c
        └── OTA.h
```

---

## 🚀 快速开始

### 环境要求
- VS Code + PlatformIO 插件
- ESP32-S3 硬件终端及对应外设
- Python 3.x（用于本地 HTTP OTA 服务）

### 部署步骤
1. 克隆项目到本地
2. 修改 `include/config.h` 中的配置项：
   - WiFi SSID 与密码（宏定义）
   - 和风天气 API Key 与城市 ID
   - EMQX Broker 地址与端口
3. 连接开发板，执行编译与烧录
```bash
pio run -t upload
```
4. 打开串口监视器查看运行日志
```bash
pio device monitor
```

---

## 🔄 OTA 升级

### 升级流程
```
设置页点击"检查更新"
        │
        ▼
gui_task 发送事件 → ota_task 唤醒执行
        │
        ▼
OTA 任务执行升级逻辑
  ├── HTTP GET 拉取固件 bin 文件
  ├── 逐块写入非当前运行 OTA 分区
  ├── 写入完成 → SHA256 完整性校验
  ├── 设置下次启动分区
  └── 设备自动重启
        │
        ▼
新固件启动 → 标记固件有效，取消回滚
  └── 若启动异常 → 下次重启自动回滚至旧版本
```

### 本地升级操作
1. 修改 `CMakeLists.txt` 中的固件版本号，执行编译
```bash
pio run
```
2. 进入固件目录，启动本地 HTTP 服务器
```bash
cd .pio/build/esp32-s3-devkitc-1-n16r8
python -m http.server 8080
```
3. 设备端进入设置页，点击 OTA 升级按钮，等待下载完成自动重启

---

## 🐛 调试与优化

| 问题现象 | 根因分析 | 优化方案 |
|----------|----------|----------|
| 屏幕出现闪烁黑线 | WiFi 射频干扰 SPI 总线 | 背光限制 25% + WiFi 发射功率限制 2dBm |
| 屏幕颜色偏色 | RGB 字节序不匹配 | 配置 `rgb_ele_order = BGR` |
| 触摸无响应 | I2C 地址冲突或总线错误 | 确认设备地址，I2C 速率降至 400KHz |
| 任务栈溢出 | 大内存开销函数调用层级深 | 对应任务栈扩容，避免大数组栈上分配 |
| OTA 升级失败 | 网络波动或分区错误 | 增加断点续传容错，校验分区地址 |

---

## 📌 开发阶段规划

按依赖关系分 8 个阶段推进，每阶段有明确验收标准：

| 阶段 | 核心内容 | 验收标准 |
|------|----------|----------|
| **P1 驱动层** | 屏幕点亮 + 触摸驱动 + LVGL 基础 Demo | 屏幕正常显示，触摸交互响应准确 |
| **P2 UI 框架** | LVGL 三页面布局 + 切换交互 | 主页/曲线/设置页可流畅切换，布局占位完成 |
| **P3 传感器** | I2C 总线驱动 + 温湿度、光照采集 | 每秒采样刷新，数据正确显示到主页 |
| **P4 网络基础** | WiFi 连接 + NTP 校时 + 和风天气 | 主页显示真实时间与天气数据，断线自动重连 |
| **P5 MQTT 互联** | EMQX 对接 + LWT/QoS/Retained 特性 | 传感器数据上云，下行指令生效，离线遗嘱触发 |
| **P6 控制与存储** | 继电器控制 + NVS 参数保存 | 本地/远程双路控制继电器，设置掉电不丢失 |
| **P7 OTA 升级** | 双分区 OTA + 回滚机制 | HTTP 升级成功，异常场景可自动回滚 |
| **P8 联调优化** | 全功能联调 + 健壮性测试 | 全功能稳定运行，异常场景容错正常 |
