# 智能新风系统主机 - 开发者指南

> **版本**: v1.0  
> **更新日期**: 2025-12-09  
> **目标读者**: 开发者、AI 助手

---

## 📚 目录

- [快速开始](#快速开始)
- [项目结构](#项目结构)
- [开发规范](#开发规范)
- [编译烧录](#编译烧录)
- [调试方法](#调试方法)
- [常见问题](#常见问题)

---

## 🚀 快速开始

### 环境要求

- **IDE**: Keil MDK 5.x
- **调试器**: JLINK / STLINK
- **MCU**: HT32F52352 (BM53A367A 开发板)
- **Pack**: HT32_DFP (Device Family Pack)

### 编译步骤

1. **打开工程**
   - 双击 `Host_MCU.uvprojx` 打开 Keil 工程

2. **选择目标**
   - 确认 Target 选择正确

3. **编译**
   - 点击 `Build (F7)` 编译
   - 确认 0 Error, 0 Warning

4. **下载**
   - 连接调试器
   - 点击 `Download (F8)` 下载到芯片

---

## 📂 项目结构

```
Host_MCU/
├── .agent/                 # AI 工作流
│   └── workflows/          # 标准化工作流
├── User/                   # 主程序与核心业务逻辑
│   ├── main.c              # 主程序入口
│   ├── app_globals.c/h     # 全局数据结构 ("数据大脑")
│   ├── iaq_algorithm.c/h   # IAQ 智能算法 ("决策大脑")
│   └── ht32f5xxxx_01_it.c  # 中断处理函数
├── System/                 # 系统层模块
│   ├── Task_Scheduler.c/h  # 任务调度器 ("行动中心")
│   ├── protocol_dl20.c/h   # Zigbee 通信协议层
│   ├── bftm.c/h            # 基本定时器驱动
│   └── delay.c/h           # 延时函数
├── Hardware/               # 硬件驱动层
│   ├── dht11.c/h           # DHT11 温湿度传感器
│   ├── mq2.c/h             # MQ2 烟雾传感器
│   ├── mq5.c/h             # MQ5 煤气传感器
│   ├── hmi_serial.c/h      # HMI 串口屏驱动
│   ├── bsp_uart.c/h        # Zigbee模块串口驱动
│   ├── usart.c/h           # ESP8266/HMI 串口驱动
│   ├── uart.c/h            # ASR-PRO 语音模块驱动
│   ├── beep.c/h            # 蜂鸣器驱动
│   └── led.c/h             # LED 指示灯驱动
├── NET/                    # 网络通信层
│   ├── onenet/             # OneNET 云平台 SDK
│   ├── MQTT/               # MQTT 协议栈
│   └── CJSON/              # JSON 解析库
├── Library/                # HT32 芯片库文件
├── Config/                 # 工程配置文件
├── docs/                   # 项目文档
│   ├── 05-设计文档/        # 详细设计文档
│   └── maintenance/        # 维护文档
├── README.md               # 项目介绍
├── DEVELOPMENT.md          # 本文档
├── ARCHITECTURE.md         # 架构设计
└── FEATURES.md             # 功能清单
```

---

## 📐 开发规范

### 1. 命名规范

| 类型 | 规范 | 示例 |
|------|------|------|
| 函数 | `模块名_功能名()` | `UART_SendString()` |
| 全局变量 | `g_` 前缀 | `g_system_data` |
| 静态变量 | `s_` 前缀 | `s_moduleState` |
| 宏定义 | 全大写 + 下划线 | `MAX_BUFFER_SIZE` |
| 结构体 | `_t` 后缀 | `SystemData_t` |

### 2. 文件行数限制

| 文件类型 | 🎯 理想 | ⚠️ 警告 | ❌ 必须拆分 |
|---------|--------|---------|-----------|
| `.c` 文件 | < 400 行 | 400-600 行 | > 800 行 |
| `.h` 文件 | < 150 行 | 150-200 行 | > 300 行 |
| 函数 | < 50 行 | 50-80 行 | > 100 行 |

### 3. 注释规范

```c
/**
 * @brief  函数功能简述
 * @param  param1: 参数1说明
 * @param  param2: 参数2说明
 * @retval 返回值说明
 */
void Module_Function(uint8_t param1, uint16_t param2)
{
    // 实现代码
}
```

### 4. 模块化原则

- 每个 `.c` 文件对应一个 `.h` 文件
- 对外接口在 `.h` 中声明
- 内部函数使用 `static` 修饰
- 避免跨模块直接访问变量

---

## 🔧 编译烧录

### Keil 配置

**Options for Target → C/C++**:
- C99 Mode: ☑
- Optimization: -O2 (发布) / -O0 (调试)
- Warnings: All Warnings

**Options for Target → Debug**:
- 选择调试器: JLINK / STLINK
- Port: SW (SWD 模式)
- Max Clock: 10MHz

### 编译输出

编译成功后查看:
- Flash: xxx bytes (xx.x% Full)
- RAM: xxx bytes (xx.x% Full)

---

## 🐛 调试方法

### 串口调试

```c
// 使用 printf 输出调试信息
printf("Debug: value = %d\r\n", value);
```

### Keil 调试

参考 [2-04-keil-debug.md](./.agent/workflows/2-04-keil-debug.md)

- 断点调试
- Watch 窗口监视变量
- Memory 窗口查看内存
- Peripheral 窗口查看寄存器

### LED 指示

| LED | 状态 | 含义 |
|-----|------|------|
| LED1 | 闪烁 | 系统运行中 |
| LED2 | 常亮 | WiFi 连接成功 |
| LED3 | 闪烁 | Zigbee 通信中 |

---

## ❓ 常见问题

### Q1: 编译报错 "No Device Library"

**解决**: 安装 HT32 Device Family Pack
- Keil → Pack Installer → 搜索 HT32 → Install

### Q2: 下载失败

**检查项**:
- 调试器连接是否正常
- 芯片是否供电
- SWD 接线是否正确

### Q3: 程序运行异常

**调试步骤**:
1. 检查时钟配置
2. 检查外设初始化
3. 使用断点定位问题

---

## 📚 参考文档

- 📖 [架构设计](./ARCHITECTURE.md)
- ✨ [功能清单](./FEATURES.md)
- 🐛 [Bug 追踪](./docs/maintenance/BUG.md)
- 📅 [更新日志](./docs/maintenance/UPDATES.md)

---

**更新时间**: 2025-12-09
