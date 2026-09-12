# 基于 MSPM0G3507 的双闭环循迹小车

[English Documentation](README.md) | [中文说明]

---

### 1. 项目简介
本项目是一套基于 **德州仪器 (TI) MSPM0G3507** 单片机的开源高性能双闭环自主循迹小车系统。系统采用前台 100Hz 强实时中断执行增量式 PI 速度闭环与八路红外灰度连续加权质心解算，后台时间片轮询非阻塞处理慢速 I/O，可稳定应对直道巡航、连续圆弧及 90° 锐利直角弯道。

### 2. 芯片来源与基础工程
- **芯片型号**：德州仪器 LP-MSPM0G3507（ARM Cortex-M0+ 内核，主频 32MHz SYSOSC，128KB Flash，32KB SRAM）。
- **SDK 版本**：基于 **TI MSPM0 SDK v2.11**（`mspm0_sdk@2.11.00.07`）官方 `nortos/empty` 空工程模板二次开发。
- **驱动架构**：采用 TI 官方 DriverLib 固件库，所有外设、时钟和引脚复用均通过 SysConfig 图形化工具（`empty.syscfg`）统一配置生成。

### 3. 核心控制架构与功能
- **100Hz 双闭环控制（10ms 周期）**：
  - **内环速度控制**：增量式 PI 控制器（`Kp = 1.20`, `Ki = 0.40`, `Kd = 0.0`），天然具备抗积分饱和特性。引入 30ms 滑动求和窗口滤波（3 拍 10ms 增量累加），平抑 9.09 RPM 离散量化抖动；阶跃响应时间 186ms，稳态零静差。
  - **外环加权循迹**：8 路探头加权质心解算偏差（$pos2 = \frac{\sum w_i \cdot b_i}{\sum b_i} \in [-7, +7]$，0.5cm 为单位）。
  - **三分级转向控制律**：
    - **直道**（$|pos2| \le 2$）：比例差速控制（$turn = 35\% \times base \times |pos2|/2$）。
    - **连续弯**（$|pos2| = 3$）：降速至 70% 基速执行比例差速。
    - **直角弯**（$|pos2| \ge 4$）：定轴原地自旋（$\pm 40$ RPM），带迟滞回差机制（偏差缩小至 $|pos2| \le 2$ 后方退出转弯）。
    - **脱线找回**：50ms 内保持上一拍历史偏差；全白超时后沿最后视线方向定轴原地自旋搜线。
- **人机交互与通信**：
  - **0.96寸 OLED 显示屏**：200ms 降频分时刷新，实时显示探头通断状态、左右轮转速、PWM 占空比、循迹偏差与档位。
  - **双模自适应串口**：9600 波特率下无缝支持 ASCII 调试指令与 `0xA5` 二进制数据包协议。
  - **非阻塞按键（PA28）**：SysTick 10ms 采样消抖状态机，消除 5~10ms 触点机械毛刺，短按一键启停循迹。
- **工业级可靠性防护**：
  - **WWDT0 硬件看门狗**：32kHz 独立 LFOSC 时钟驱动，1.0s 超时时间，主循环定期喂狗，死锁自动硬件复位。
  - **I2C 9 时钟总线自愈**：开机主动发送 9 个 SCL 脉冲强行释放被从机拉低的 SDA，彻底避免热复位死锁。
  - **静态显存防御**：显存操作改用静态缓冲区，根除动态变长数组导致的 256B 系统栈溢出问题。
  - **TB6612 能耗制动**：目标速度归零时驱动芯片进入低边短路刹车（Brake），杜绝车体滑行溜车。

### 4. 硬件配置与引脚定义

| 模块 | 硬件选型 | 接口 / 外设 | 单片机引脚 | 功能说明 |
| :--- | :--- | :--- | :--- | :--- |
| **主控** | LP-MSPM0G3507 | Cortex-M0+ @ 32MHz | - | TI 官方 LaunchPad 开发板 |
| **电机驱动** | TB6612FNG 双 H 桥驱动板 | GPIO + TIMG8 PWM | PB22, PB23, PB15（左轮）<br>PB25, PB26, PB16（右轮） | 10kHz PWM 载波，±1000 占空比 |
| **编码器** | 11线磁电正交霍尔编码器 ×2 | GPIO 外部中断 | PA14, PB6（左轮）<br>PA17, PA18（右轮） | 20:1 减速比，单圈 220 脉冲 |
| **循迹模块** | 8路红外灰度传感器阵列 | 数字 GPIO 采集 | PB0..PB4, PB17..PB19 | 探头间距 0.5cm，识别黑线为高电平 |
| **显示器** | 0.96寸 OLED (SSD1306) | 硬件 I2C0 (400kHz) | PA0 (SDA), PA1 (SCL) | 128x64 分辨率，200ms 突发刷新 |
| **按键** | 机械轻触按键 | GPIO 弱上拉输入 | PA28 | 短按一键启停循迹导航 |
| **串口调试** | USB 转串口 (CH340) | UART0 (9600-8-N-1) | PA10 (TX), PA11 (RX) | 调试指令交互与 TLOG 遥测输出 |
| **看门狗** | 硬件独立看门狗 WWDT0 | 独立 LFOSC 时钟 | 内部外设 | 1.0s 超时硬件复位保底 |

### 5. 工程目录结构
```text
empty_nortos/
├── BSP/                    # 板级支持包驱动
│   ├── Encoder.c / .h      # 霍尔编码器脉冲计数与测速
│   ├── Gray.c / .h         # 8 路红外灰度传感器采集
│   ├── Key.c / .h          # 非阻塞按键状态机驱动
│   ├── LED.c / .h          # 板载 LED 状态指示
│   ├── Motor.c / .h        # TB6612FNG 电机底层驱动与 PWM
│   ├── OLED.c / .h         # SSD1306 I2C 显示驱动与绘图接口
│   ├── OLED_Data.c / .h    # OLED 字模与位图数据
│   ├── PID.c / .h          # 增量式 PID 控制算法实现
│   ├── PWM.c / .h          # 硬件 PWM 封装层
│   ├── SpeedCtrl.c / .h    # 双轮速度闭环控制
│   ├── Tick.c / .h         # SysTick 1ms 系统时基
│   ├── Track.c / .h        # 加权质心循迹决策与分级转向
│   ├── USART.c / .h        # 双模串口驱动与指令解析
│   └── Watchdog.c / .h     # WWDT0 硬件看门狗管理
├── assets/                 # 原理图、PCB工程及引脚分配图
├── empty.c                 # 主程序入口与主循环调度
├── empty.syscfg            # SysConfig 外设配置文件
├── keil/                   # Keil MDK 工程文件
│   └── empty_LP_MSPM0G3507_nortos_keil.uvprojx
├── source/                 # TI DriverLib 固件库及 ARM CMSIS 核心头文件
├── LICENSE                 # BSD 3-Clause 开源协议文本
├── README.md               # 英文说明文档
└── README_CN.md            # 中文说明文档
```

### 6. 编译与烧录说明

#### 源码文件编码规范（GB2312）
> **重要提示**：本工程所有源码（`.c`）与头文件（`.h`）统一采用 **GB2312 (CP936)** 字符编码与 **CRLF (`\r\n`)** 换行格式，以原生契合 Keil MDK-ARM 默认编辑器，防止中文注释出现乱码。
> 若使用 VS Code、CLion 等现代编辑器浏览或修改代码，请确保将文件打开编码设置为 `GB2312` 或 `GBK`（VS Code 用户已在 `.vscode/settings.json` 中配置默认编码）。

#### 环境要求
- **IDE**：Keil MDK-ARM (v5.38 及以上版本)，搭配 Arm Compiler 6 (ARMCLANG v6.19+)。
- **配置工具**：TI SysConfig 独立版 (v1.18+)，或 Keil 内置 SysConfig 动作。
- **支持包**：Texas Instruments MSPM0G Device Family Pack (DFP)。
- **硬件**：LP-MSPM0G3507 核心板及小车底盘总成。

#### 编译与下载
1. 使用 Keil MDK 打开 `keil/empty_LP_MSPM0G3507_nortos_keil.uvprojx`。
2. 确认工程配置已选用 **Arm Compiler 6**。
3. 点击 **Build** (F7) 进行全工程编译，生成 `.axf` 与 `.hex` 固件（0 错误，0 警告）。
4. 通过板载 XDS110 或 DAP-Link 调试器连接开发板，点击 **Download** (F8) 完成固件烧录。

#### 串口控制指令（9600 波特率）
| 指令格式 | 示例 | 功能说明 |
| :--- | :--- | :--- |
| `SPD=<rpm>` | `SPD=100` | 设定左右轮闭环目标转速（RPM） |
| `KP=<val>` | `KP=1.2` | 动态修改速度环比例系数 Kp |
| `KI=<val>` | `KI=0.4` | 动态修改速度环积分系数 Ki |
| `TRK=<0/1>` | `TRK=1` | 启动 (`1`) 或停止 (`0`) 自主循迹 |
| `TSPD=<rpm>` | `TSPD=100` | 设定循迹直道基准巡航转速（默认 100 RPM） |
| `TLOG=<ms>` | `TLOG=50` | 以 `<ms>` 周期开启高速循迹遥测输出（`0` 为关闭） |
| `RST?` | `RST?` | 查询单片机本次开机复位原因 |
| `WDT_TEST` | `WDT_TEST` | 故意制造系统死锁以测试硬件看门狗复位 |

---

### 7. 开源协议与第三方声明（License & Disclaimers）

本项目采用 **BSD 3-Clause License** 开源协议。完整协议内容参见 [LICENSE](LICENSE) 文件。

- **德州仪器（Texas Instruments）**：DriverLib 固件库及 SysConfig 文件受德州仪器版权保护，采用 **BSD 3-Clause License**。
- **安谋国际（Arm Limited CMSIS）**：ARM CMSIS Core 核心头文件受 Arm Limited 版权保护，采用 **Apache-2.0 License**。
- **Keil / Arm 工具链与商标声明**：*Arm*、*Keil* 和 *MDK-ARM* 是 Arm Limited（或其子公司）在美欧及其他国家/地区的注册商标。本项目为独立的社区开源项目，与 Arm 或 Keil 无官方商业关联或品牌代言。本项目源码及工程配置（`.uvprojx`）中不包含任何 Keil 商业软件本体、编译器可执行程序或破解文件。使用 Keil MDK-ARM 编译本项目需要开发者自行遵循 Arm 的软件最终用户许可协议（EULA）并获取合法授权（非商业学习/开源用途可免费申请并使用官方 Keil MDK Community 社区版）。
