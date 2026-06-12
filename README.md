
```markdown
# ESP32-S3 Smart Desktop IoT Terminal

> **ESP32-S3 + LVGL v9.5 + FreeRTOS + EMQX — 多功能 IoT 桌面终端**
> 
> 温湿度/光照采集 · 和风天气 · NTP 时钟 · MQTT 远程控制 · OTA 升级

---

## 🧱 硬件

| 组件 | 型号 | GPIO |
|------|------|------|
| 主控 | ESP32-S3 (16MB Flash / 8MB PSRAM) | — |
| 屏幕 | ILI9341 2.8" 240×320 | SPI: CLK=15 MOSI=7 DC=6 RST=5 CS=4 |
| 背光 | PWM | 8 |
| 触摸 | FT6336 | I2C_NUM_1: SDA=10 SCL=9 RST=12 INT=11 |
| 温湿度 | SHT30 | I2C: SDA=1 SCL=2 |
| 光照 | BH1750 | I2C: SDA=1 SCL=2 |
| 继电器 | High trigger | 14 |

---

## ⚙️ 软件架构

```
Time_Weather (5)   Gui (2)   Sensor (1)   Network (3)   OTA (4)
     │                 │          │             │           │
     │                 │    ┌─────┴─────┐       │           │
     │                 │    │ xQueue    │       │           │
     │                 │    │ Overwrite │       │           │
     │                 │    └─────┬─────┘       │           │
     │                 │         │               │           │
     ▼                 ▼         ▼               ▼           ▼
  NTP + 天气        LVGL UI   读写 SHT30    EMQX MQTT    HTTP 下载
                             BH1750        上传+下行     固件 OTA
```

---

## ✅ 功能清单

- [x] LVGL v9.5 三页 UI (Home / Chart / Setting)
- [x] SHT30 温湿度 + BH1750 光照, 每秒采集
- [x] WiFi STA 自动重连
- [x] NTP 东八区时间 + 和风天气 API (gzip)
- [x] EMQX MQTT: **LWT 遗嘱 + QoS 分级 + Retained 消息**
- [x] 下行控制: 继电器开关 / 阈值调节 / 滑块实时同步
- [x] NVS 存储: 温度阈值 / 湿度阈值 / 背光亮度
- [x] OTA 双分区升级 + 回滚保护

---

## 🚀 快速开始

```bash
# 1. 修改 include/config.h 中的 WiFi 和 API 密钥
# 2. 编译烧录
pio run -t upload -t monitor
```

---

## 🔄 OTA 升级

```bash
# 修改 CMakeLists.txt 版本号 → pio run 编译
# 启动 HTTP 服务器
cd .pio/build/esp32-s3-devkitc-1-n16r8
python -m http.server 8080

# 设备 Setting 页 → Check Update → 下载 → 自动重启
```

---

## 🐛 调试日志

| 问题 | 根因 | 解决 |
|------|------|------|
| 颜色偏绿 | RGB → BGR | `rgb_ele_order = BGR` |
| 触摸不通 | I2C_NUM_0 冲突 | 改用 I2C_NUM_1 |
| 黑线闪烁 | WiFi 射频干扰 SPI | 降低发射功率 + SPI 降频 |
| 栈溢出 | gzip 解压栈过深 | 任务栈 16KB |
| OTA 崩溃 | Queue 长度≠1 | `xQueueCreate(1, …)` |

---

## 📂 项目结构

```
├── platformio.ini
├── partitions.csv
├── CMakeLists.txt
├── include/config.h
├── src/
│   ├── main.c
│   ├── FreeRTOS_Task/
│   ├── Device/{LCD, BH1750, SHT30, Relay}
│   ├── IOT/{EMQX, WIFI_Init, Get_Weather_Time, OTA}
│   ├── UI/
│   ├── NVS/
│   └── MY_I2C/
```
