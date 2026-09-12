# MSPM0G3507 Dual-Loop Smart Tracking Car

[English] | [中文说明](README_CN.md)

---

### 1. Overview
This project is an open-source, dual-closed-loop autonomous tracking car system built on the **Texas Instruments (TI) MSPM0G3507** microcontroller. It integrates a 100Hz incremental PI wheel speed control loop with an 8-channel infrared grayscale continuous weighted-centroid tracking loop, achieving high-stability line tracking across straight lines, continuous curves, and 90° right-angle turns.

### 2. Baseline & Chip Information
- **MCU**: Texas Instruments LP-MSPM0G3507 (ARM Cortex-M0+ core running at 32MHz SYSOSC, 128KB Flash, 32KB SRAM).
- **SDK Baseline**: Developed from the `nortos/empty` starter template of **TI MSPM0 SDK v2.11** (`mspm0_sdk@2.11.00.07`).
- **Driver Layer**: Built on TI MSPM0 DriverLib, with peripherals configured via TI SysConfig (`empty.syscfg`).

### 3. Key Architecture & Features
- **Dual Closed-Loop Control (100Hz / 10ms cycle)**:
  - **Inner Speed Loop**: Incremental PI controller with velocity-form anti-windup (`Kp = 1.20`, `Ki = 0.40`, `Kd = 0.0`). Uses a 30ms moving-average window (3 x 10ms samples) to eliminate 9.09 RPM encoder discrete quantization noise. Step response settles within 186ms with zero steady-state error.
  - **Outer Tracking Loop**: Continuous weighted centroid calculation over 8 infrared grayscale probes ($pos2 = \frac{\sum w_i \cdot b_i}{\sum b_i} \in [-7, +7]$ in 0.5cm units).
  - **3-Gear Steering Law**:
    - **Straight** ($|pos2| \le 2$): Proportional differential steering ($turn = 35\% \times base \times |pos2|/2$).
    - **Continuous Curve** ($|pos2| = 3$): Differential steering at 70% cruise speed.
    - **90° Right Angle** ($|pos2| \ge 4$): In-place fixed-axis spin ($\pm 40$ RPM) with exit hysteresis requirement ($|pos2| \le 2$).
    - **Off-line Recovery**: 50ms historical deviation hold, followed by fixed-axis spin search toward the last observed line direction.
- **Human-Machine Interface & Telemetry**:
  - **0.96" SSD1306 OLED (I2C 400kHz)**: 200ms burst page update displaying 8-probe sensor states, wheel RPM, PWM duty cycles, line deviation, and active gear.
  - **Dual-Mode UART0 (9600-8-N-1)**: Transparently supports human-readable ASCII commands and binary telemetry (`0xA5` frame header + checksum + `0x5A` frame tail).
  - **Non-blocking Button (PA28)**: 10ms SysTick polling state machine debounces mechanical chatter (5-10ms) without blocking delays.
- **Industrial-Grade Fault Tolerance**:
  - **Hardware Window Watchdog (WWDT0)**: Driven by independent 32kHz LFOSC, 1.0s timeout to auto-recover from system hangs.
  - **9-Clock I2C Bus Auto-Recovery**: Generates 9 SCL recovery clock pulses on boot to release stuck-low SDA slave conditions.
  - **Static Display Buffer**: Uses fixed static array in display routine to prevent stack overflow on constrained 256-byte stacks.
  - **Active Low-Side Braking**: TB6612FNG short-circuit braking when speed targets reach zero.

### 4. Hardware Specifications & Pin Mapping

| Module | Hardware Component | Interface / Peripheral | MCU Pins | Description |
| :--- | :--- | :--- | :--- | :--- |
| **MCU** | LP-MSPM0G3507 | Cortex-M0+ @ 32MHz | - | TI MSPM0 LaunchPad |
| **Motor Driver** | TB6612FNG Dual H-Bridge | GPIO + TIMG8 PWM | PB22, PB23, PB15 (Left)<br>PB25, PB26, PB16 (Right) | 10kHz PWM carrier, ±1000 duty |
| **Encoders** | Dual 11-PPR Hall Encoders | GPIO External Interrupt | PA14, PB6 (Left)<br>PA17, PA18 (Right) | 20:1 gearbox, 220 pulses/rev |
| **Line Sensor** | 8-Channel IR Grayscale Array | Digital GPIO | PB0..PB4, PB17..PB19 | 0.5cm probe pitch, active-high black line |
| **Display** | 0.96" OLED (SSD1306) | Hardware I2C0 (400kHz) | PA0 (SDA), PA1 (SCL) | 128x64 pixels, 200ms refresh rate |
| **Key** | Push Button | GPIO Input (Internal Pull-Up)| PA28 | Short press toggles tracking |
| **Serial Debug** | USB-to-UART (CH340) | UART0 (9600-8-N-1) | PA10 (TX), PA11 (RX) | Command control & TLOG telemetry |
| **Watchdog** | Hardware WWDT0 | Independent LFOSC | Internal | 1.0s hardware reset timeout |

### 5. Project Structure
```text
empty_nortos/
├── BSP/                    # Board Support Package drivers
│   ├── Encoder.c / .h      # M-method velocity measurement & pulse counting
│   ├── Gray.c / .h         # 8-channel infrared grayscale sensor driver
│   ├── Key.c / .h          # Non-blocking button debouncing state machine
│   ├── LED.c / .h          # Status LED driver
│   ├── Motor.c / .h        # TB6612FNG motor driver & PWM control
│   ├── OLED.c / .h         # SSD1306 I2C driver & graphics primitives
│   ├── OLED_Data.c / .h    # Font glyphs and display bitmaps
│   ├── PID.c / .h          # Incremental PID controller algorithm
│   ├── PWM.c / .h          # Hardware PWM wrapper
│   ├── SpeedCtrl.c / .h    # Dual-wheel speed closed-loop controller
│   ├── Tick.c / .h         # SysTick 1ms system timebase
│   ├── Track.c / .h        # Weighted centroid tracking & steering decision
│   ├── USART.c / .h        # Dual-mode UART driver & command parser
│   └── Watchdog.c / .h     # WWDT0 hardware watchdog driver
├── assets/                 # Schematics, PCB designs, and pinout diagrams
├── empty.c                 # Application entry and main loop
├── empty.syscfg            # SysConfig peripheral configuration file
├── keil/                   # Keil MDK project files
│   └── empty_LP_MSPM0G3507_nortos_keil.uvprojx
├── source/                 # TI DriverLib SDK and ARM CMSIS Core headers
├── LICENSE                 # BSD 3-Clause open source license
├── README.md               # English documentation
└── README_CN.md            # Chinese documentation
```

### 6. Getting Started

#### Source Code Encoding (GB2312)
> **Important Note**: All C source files (`.c`) and header files (`.h`) in this project are strictly unified under **GB2312 (CP936)** encoding with **CRLF (`\r\n`)** line endings. This ensures native out-of-the-box compatibility with Keil MDK-ARM's internal editor without Chinese character corruption.
> If you are using VS Code, CLion, or other modern editors, please ensure your editor's file encoding is set to `GB2312` (or `GBK`).

#### Requirements
- **IDE**: Keil MDK-ARM v5.38 or later with ARMCLANG (Arm Compiler 6.19+).
- **Tool**: TI SysConfig standalone v1.18+ (or Keil integrated action).
- **Pack**: Texas Instruments MSPM0G Device Family Pack (DFP).
- **Hardware**: LP-MSPM0G3507 LaunchPad with chassis, motors, sensors, and OLED.

#### Build & Flash
1. Open `keil/empty_LP_MSPM0G3507_nortos_keil.uvprojx` in Keil MDK.
2. If modifying peripherals, edit `empty.syscfg` in SysConfig to regenerate configuration files.
3. Click **Build** (F7). Ensure the project compiles with 0 errors and 0 warnings.
4. Connect LP-MSPM0G3507 via USB and click **Download** (F8) to flash.

#### Serial Commands (9600 Baud)
| Command | Example | Description |
| :--- | :--- | :--- |
| `SPD=<rpm>` | `SPD=100` | Set closed-loop target speed for both wheels (RPM) |
| `KP=<val>` | `KP=1.2` | Dynamically update speed loop proportional gain |
| `KI=<val>` | `KI=0.4` | Dynamically update speed loop integral gain |
| `TRK=<0/1>` | `TRK=1` | Enable (`1`) or disable (`0`) autonomous tracking |
| `TSPD=<rpm>` | `TSPD=100` | Set tracking cruising base speed (default 100 RPM) |
| `TLOG=<ms>` | `TLOG=50` | Stream compact telemetry at `<ms>` interval (`0` to stop) |
| `RST?` | `RST?` | Query the reason for the last MCU reset |
| `WDT_TEST` | `WDT_TEST` | Trigger an intentional lockup to verify watchdog reset |

---

### 7. License & Disclaimers

This project is licensed under the **BSD 3-Clause License**. See the [LICENSE](LICENSE) file for complete terms.

- **Texas Instruments**: DriverLib and SysConfig files are licensed under the **BSD 3-Clause License** by Texas Instruments Incorporated.
- **ARM CMSIS Core**: CMSIS Core header files are licensed under the **Apache-2.0 License** by Arm Limited.
- **Keil / Arm Toolchain Notice**: *Arm*, *Keil*, and *MDK-ARM* are registered trademarks of Arm Limited. This project is an independent open-source community effort and is not endorsed by or affiliated with Arm Limited. Compiling this project using Keil MDK-ARM requires users to maintain their own valid Keil MDK software license (e.g., Keil MDK Community edition for non-commercial learning/open-source use) in compliance with Arm's End User License Agreement (EULA). This repository does not distribute or bundle any Keil proprietary software or licenses.
