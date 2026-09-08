/**
 * @file    UART.h
 * @brief   MSPM0G3507 串口通信驱动与控制协议规范
 * @details 定义 UART0 (UART_Debug) 硬件通信接口及完整的上位机控制指令集。
 *
 * =============================================================================
 *                       UART 通信协议与控制指令手册
 * =============================================================================
 *
 * 1. 物理层与通信参数配置 (Hardware & Communication Settings)
 * -----------------------------------------------------------------------------
 *   - 硬件接口:   UART0 (UART_Debug_INST)
 *   - 通信波特率: 9600 bps
 *   - 数据位:     8 bit
 *   - 校验位:     无校验 (None)
 *   - 停止位:     1 bit
 *   - 硬件流控:   无 (None)
 *   - 接收缓冲区: 100 字节 (USART_t.rxbuff)
 *   - 帧结束符:   '\r' (CR, 0x0D)、'\n' (LF, 0x0A) 或 "\r\n"
 *   - 帧超时机制: 当接收缓冲区有字符且超过 100ms 未收到帧尾字符时，强制成帧解析
 *   - 响应与回显: 收到合法数据帧后，MCU 会首先原样回显接收内容并换行，
 *                 随后输出指令执行应答（以 "[MCU OK]" 格式开头）：
 *                   [步骤1 - 回显] <接收指令内容>\r\n
 *                   [步骤2 - 应答] [MCU OK] ...\r\n
 *
 * 2. 串口控制指令详表 (Supported Commands Matrix)
 * -----------------------------------------------------------------------------
 *
 * 【一、开环 PWM 与方向控制 (Open-Loop Control)】
 *   适用于硬件底层驱动验证、开环无反馈调速与转向测试。
 *
 *   (1) PWM 占空比设定
 *       - 指令语法: PWM=<0~1000> 或 pwm=<0~1000> (大小写不敏感)
 *       - 参数范围: 0 ~ 1000 (无量纲整数，对应系统 PWM Period=1000，即 0% ~ 100% 占空比)
 *       - MCU 应答: "[MCU OK] PWM=%d, Dir=%s\r\n"
 *                   例: 输入 "PWM=500\r\n" -> 应答 "[MCU OK] PWM=500, Dir=+\r\n"
 *       - 功能说明: 设定左右电机 PWM 输出占空比，并根据当前方向标志输出相应电平。
 *
 *   (2) 正向旋转 (前进)
 *       - 指令语法: + (可单独成帧，或与 PWM 拼合如 "+PWM=300" / "pwm=300+")
 *       - 参数说明: 无参数
 *       - MCU 应答: "[MCU OK] PWM=%d, Dir=+\r\n"
 *       - 功能说明: 设置电机运动方向为正转 / 前进 (Font = 0)。
 *
 *   (3) 反向旋转 (后退)
 *       - 指令语法: - (可单独成帧，或与 PWM 拼合如 "-PWM=300" / "pwm=300-")
 *       - 参数说明: 无参数
 *       - MCU 应答: "[MCU OK] PWM=%d, Dir=-\r\n"
 *       - 功能说明: 设置电机运动方向为反转 / 后退 (Font = 1)。
 *
 * 【二、速度闭环控制与 PID 调参 (Speed Closed-Loop & PID Control)】
 *   适用于定速巡航、抗载荷扰动恒速行驶及增量式 PID 控制算法参数在线标定。
 *
 *   (1) 速度闭环启停切换
 *       - 指令语法: PID=1 / PID=0 或 pid=1 / pid=0 (大小写不敏感)
 *       - 参数范围: 1: 使能闭环; 0: 关闭闭环 (整数)
 *       - MCU 应答: "[MCU OK] PID enable=%d\r\n" (1 或 0)
 *       - 功能说明: PID=1 启动 10ms 周期的增量式 PID 闭环调速；
 *                   PID=0 关闭速度闭环，左右电机自动滑行停转，系统切回开环待机状态。
 *
 *   (2) 双轮闭环目标转速设定
 *       - 指令语法: SPD=<-400~400> 或 spd=<-400~400> (大小写不敏感)
 *       - 参数范围: -400 ~ +400 (整数，单位: RPM，代表电机减速箱输出轴实际转速)
 *                   正数表示正转巡航，负数表示反转巡航，0 表示闭环制动锁轴。
 *       - MCU 应答: "[MCU OK] PID target=%d RPM\r\n"
 *                   例: 输入 "SPD=120\r\n" -> 应答 "[MCU OK] PID target=120 RPM\r\n"
 *       - 功能说明: 设定左右轮统一的目标转速，闭环控制器自动调节左右电机 PWM 追踪目标。
 *
 *   (3) PID 比例增益 (Kp) 调节
 *       - 指令语法: KP=<float> 或 kp=<float> (大小写不敏感)
 *       - 参数范围: 浮点数 (支持正负号与最多3位小数，例如 KP=1.5、kp=0.85)
 *       - MCU 应答: "[MCU OK] KP=%d (x0.001)\r\n"
 *                   例: 输入 "KP=1.5\r\n" -> 应答 "[MCU OK] KP=1500 (x0.001)\r\n"
 *       - 功能说明: 调整增量式 PID 的比例增益系数 Kp。Kp 提高动态响应速度，过大易引发振荡。
 *
 *   (4) PID 积分增益 (Ki) 调节
 *       - 指令语法: KI=<float> 或 ki=<float> (大小写不敏感)
 *       - 参数范围: 浮点数 (支持正负号与最多3位小数，例如 KI=0.2、ki=0.05)
 *       - MCU 应答: "[MCU OK] KI=%d (x0.001)\r\n"
 *                   例: 输入 "KI=0.2\r\n" -> 应答 "[MCU OK] KI=200 (x0.001)\r\n"
 *       - 功能说明: 调整增量式 PID 的积分增益系数 Ki。用于消除稳态转速误差。
 *
 *   (5) PID 微分增益 (Kd) 调节
 *       - 指令语法: KD=<float> 或 kd=<float> (大小写不敏感)
 *       - 参数范围: 浮点数 (支持正负号与最多3位小数，例如 KD=0.1、kd=0.02)
 *       - MCU 应答: "[MCU OK] KD=%d (x0.001)\r\n"
 *                   例: 输入 "KD=0.1\r\n" -> 应答 "[MCU OK] KD=100 (x0.001)\r\n"
 *       - 功能说明: 调整增量式 PID 的微分增益系数 Kd。用于抑制动态超调并提高阻尼特性。
 *
 * 【三、编码器脉冲与线数标定 (Encoder Calibration)】
 *   适用于轮组实际减速比与光电/霍尔编码器物理线数 (PPR) 实车测试标定。
 *
 *   (1) 读取并清零编码器原始累计脉冲
 *       - 指令语法: CNT 或 cnt (大小写不敏感，无参数)
 *       - MCU 应答: "[MCU OK] CNT RL=%d RR=%d (1 output rev / 20 = PPR)\r\n"
 *                   例: "[MCU OK] CNT RL=220 RR=220 (1 output rev / 20 = PPR)\r\n"
 *       - 功能说明: 读取自上次读取以来左轮(RL)和右轮(RR)的原始编码器计数值，并立即清零。
 *       - 标定操作指南:
 *           1. 车体架空，通过串口发送一次 "CNT" 清除历史计数值；
 *           2. 手动将输出轮精准匀速旋转 1 整圈 (1 output rev)；
 *           3. 再次发送 "CNT" 指令读取脉冲累加数；
 *           4. 实测 PPR 计算公式: PPR = 读出脉冲数 / 减速比(20)。
 *
 * 【四、自主寻线跟踪控制 (Autonomous Line Tracking)】
 *   适用于基于 8 路红外灰度传感器阵列的全自主黑线寻迹行驶模式。
 *
 *   (1) 循迹控制启停使能
 *       - 指令语法: TRK=1 / TRK=0 或 trk=1 / trk=0 (大小写不敏感)
 *       - 参数范围: 1: 开启循迹; 0: 关闭循迹 (整数)
 *       - MCU 应答: "[MCU OK] Track enable=%d\r\n" (1 或 0)
 *       - 功能说明: TRK=1 时自动联动启动速度闭环控制 (SpeedCtrl_Enable(1)) 并运行
 *                   灰度循迹状态机；TRK=0 时关闭循迹并联动关闭闭环 (SpeedCtrl_Enable(0))。
 *
 *   (2) 循迹巡航基准转速设定
 *       - 指令语法: TSPD=<0~250> 或 tspd=<0~250> (大小写不敏感)
 *       - 参数范围: 0 ~ 250 (整数，单位: RPM，默认预设基准转速为 100 RPM)
 *       - MCU 应答: "[MCU OK] Track base speed=%d RPM\r\n"
 *                   例: 输入 "TSPD=120\r\n" -> 应答 "[MCU OK] Track base speed=120 RPM\r\n"
 *       - 功能说明: 设定自主循迹直道巡航的基准线速度。循迹控制算法根据黑线归一化
 *                   偏差 pos_norm [-1.0, +1.0] 在此基准速度上叠加差速调节量。
 *
 * 【五、周期状态遥测报文 (Periodic Telemetry & Diagnostics)】
 *   - 触发方式: 主循环中周期调用 UART_Poll_MotorStatus(period_ms) 自动发出 (默认周期 500ms)。
 *   - 报文格式:
 *       "PWM L:%d R:%d Dir:%s | RPM L:%d R:%d | RX:%u C:%u D:%02X %02X %02X %02X\r\n"
 *   - 字段说明:
 *       - PWM L/R: 左右电机当前 PWM 设定值 (-1000 ~ +1000)
 *       - Dir: 当前运行方向 (+ 前进 / - 后退)
 *       - RPM L/R: 左右轮编码器测算的当前实际输出轴转速 (RPM)
 *       - RX: 系统自上电/复位后 UART 硬件接收到的总字节计数 (用于排查通信链路)
 *       - C: 当前接收缓冲区未处理字节计数
 *       - D: 缓冲区前 4 个字节的十六进制显示 (辅助协议嗅探与对齐排查)
 * =============================================================================
 */

#ifndef __UART_H
#define __UART_H

#include "ti_msp_dl_config.h"
#include <stdint.h>
#include <stdbool.h>

/* 串口通信结构体 */
typedef struct {
    uint8_t rxbuff[100];    /* 接收缓冲区 */
    uint8_t rxcount;        /* 接收字符计数器 */
    uint8_t rxover;         /* 接收完成标志位 (1: 一帧接收完毕) */
    uint8_t txbuff[100];    /* 发送缓冲区 */
} USART_t;

/* 全局串口结构体与电机状态变量 */
extern USART_t myusart;
extern uint16_t Compare;    /* 速度占空比 (0 ~ 1000) */
extern bool Font;           /* 方向: 0 为正转 (+/Forward), 1 为反转 (-/Backward) */

/**
 * @brief UART_Debug (UART0) 串口初始化函数
 */
void UART_Init(void);

/**
 * @brief 串口发送单个字符（标准阻塞发送）
 * @param ch 要发送的字符
 */
void UART_Send_Byte(char ch);

/**
 * @brief 串口发送字符串
 * @param str 以 '\0' 结尾的字符串首地址
 */
void UART_Send_Str(char *str);

/**
 * @brief 串口发送指定长度的缓冲区
 * @param str   缓冲区首地址
 * @param lenth 要发送的字节数
 */
void UART_Send_Buff(uint8_t *str, uint8_t lenth);

/**
 * @brief 串口接收数据解析函数
 * @note  根据接收缓冲区内容解析开环 PWM、闭环速度 PID、编码器标定、自主循迹等指令。
 *        支持指令详见文件顶部指令手册。
 */
void Data_Anylize(void);

/**
 * @brief 帧处理函数: 检测到一帧接收完成后 (rxover=1), 在主循环中完成回显、解析与应答
 * @note  需在主循环中周期调用; 中断内仅收帧置标志, 不做耗时操作
 */
void UART_ProcessFrame(void);

/**
 * @brief 获取 UART 自上电以来收到的总字节数 (诊断 RX 链路用)
 * @return uint32_t 累计接收字节数
 */
uint32_t UART_GetRxTotal(void);

/**
 * @brief 串口轮询发送电机状态（左速度、右速度、方向），合成一条字符串周期发出
 * @param period_ms 发送周期，单位毫秒（建议 500ms）
 */
void UART_Poll_MotorStatus(uint32_t period_ms);

#endif /* __UART_H */
