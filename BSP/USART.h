/**
 * @file    USART.h
 * @brief   MSPM0G3507 统一串口通信模块 (融合定长/变长二进制 HEX 数据包与 PC ASCII 文本指令)
 * @details 硬件基于 UART0 (UART_Debug_INST, 9600-8-N-1)，实现双模自动识别接收与无阻塞主循环处理。
 *
 * =============================================================================
 *                      USART 模块通信规范与协议速查
 * =============================================================================
 *
 * 一、二进制 HEX 数据包协议 (用于视觉模块 K230 / 下位机传感器等低延迟传输)
 * -----------------------------------------------------------------------------
 *   - 帧头 (Header): 0xA5
 *   - 帧尾 (Tail):   0x5A
 *   - 校验和 (SUM):  低 8 位算术累加和 (uint8_t)
 *
 *   1. 定长 5 字节数据包 (Fixed-length Packet):
 *      [0xA5] [D1] [D2] [SUM = (uint8_t)(D1 + D2)] [0x5A]
 *      - 载荷长度: 固定 2 字节
 *      - 发送 API: USART_SendPacket_Fixed(d1, d2)
 *      - 读取 API: USART_GetFixedPacket(&d1, &d2)
 *
 *   2. 不定长 (变长) 数据包 (Variable-length Packet):
 *      [0xA5] [LEN] [DATA_0 ... DATA_LEN-1] [SUM = sum(DATA)] [0x5A]
 *      - 载荷长度: LEN (0 ~ 60 字节)
 *      - 总帧长:   LEN + 4 字节
 *      - 校验和:   仅累加数据位 (DATA_0 ~ DATA_LEN-1)，不包含 LEN
 *      - 发送 API: USART_SendPacket_Var(data, len)
 *      - 读取 API: USART_GetPacket(out_data, &out_len)
 *
 *   3. 状态查询与清理 API:
 *      - bool USART_IsPacketReady(void);
 *      - void USART_ClearPacket(void);
 *
 * 二、PC ASCII 文本控制指令集 (用于上位机/电脑串口助手调试交互)
 * -----------------------------------------------------------------------------
 *   - 帧定界符: '\r' (0x0D)、'\n' (0x0A) 或 "\r\n"
 *   - 帧超时:   100ms 强制封帧保护 (防止丢尾卡死)
 *   - 指令列表:
 *       1. PWM=<0~1000> / pwm=<0~1000> : 设置开环 PWM 占空比
 *       2. + / -                       : 设置电机开环方向 (+ 正转 / - 反转)
 *       3. Sp<val> / sp<val>           : 兼容旧版开环调速 (0~1000)
 *       4. PID=1 / PID=0               : 启闭速度闭环控制器
 *       5. SPD=<-400~400>              : 设置闭环双轮目标转速 (RPM)
 *       6. KP=<float>, KI=<f>, KD=<f>  : 在线标定增量式 PID 控制参数
 *       7. CNT                         : 查询并清零左右轮编码器累计脉冲数 (PPR 标定)
 *       8. TRK=1 / TRK=0               : 启闭自主循迹模式 (同步联动速度闭环)
 *       9. TSPD=<0~250>                : 设置循迹巡航基准转速 (RPM)
 *
 * 三、双模自动识别与非阻塞架构
 * -----------------------------------------------------------------------------
 *   - 中断服务函数 (UART_Debug_INST_IRQHandler):
 *       循环清空 RX FIFO，遇到 0xA5 自动切入 HEX 数据包状态机；其他字符流缓冲为 ASCII 字符串。
 *   - 主循环处理 (USART_ProcessFrame):
 *       非阻塞执行 ASCII 指令解析、回显并派发到对应业务模块。
 *   - 定时遥测 (USART_Poll_MotorStatus):
 *       周期性格式化发送左右轮 PWM、RPM 与循迹状态。
 * =============================================================================
 */

#ifndef __USART_H
#define __USART_H

#include "ti_msp_dl_config.h"
#include <stdint.h>
#include <stdbool.h>

#define USART_RXMAX             100     /* ASCII 文本指令接收缓冲区大小 */
#define USART_HEX_BUFF_MAX      64      /* HEX 数据包接收临时缓冲区大小 */
#define USART_PACKET_MAX        60      /* HEX 数据包最大数据载荷 (Payload) 长度 */

#define USART_FRAME_HEAD        0xA5    /* 二进制 HEX 帧头 */
#define USART_FRAME_TAIL        0x5A    /* 二进制 HEX 帧尾 */

/* 统一串口运行结构体 */
typedef struct {
    /* ASCII 文本协议 */
    uint8_t rxbuff[USART_RXMAX];    /* ASCII 接收缓冲区 */
    uint8_t rxcount;                /* ASCII 字符接收计数 */
    uint8_t rxover;                 /* ASCII 接收完成标志 (1: 完成) */
    uint8_t txbuff[128];            /* 发送格式化缓冲区 */

    /* 二进制 HEX 数据包协议 */
    uint8_t hex_buff[USART_HEX_BUFF_MAX]; /* 接收状态机内部缓冲 */
    uint8_t hex_count;                    /* 接收状态机字节计数 */
    uint8_t hex_state;                    /* 状态机状态: 0-等待帧头, 1-接收中 */
    uint8_t packet_data[USART_PACKET_MAX];/* 最新有效载荷数据 (Payload) */
    uint8_t packet_len;                   /* 有效载荷字节数 */
    bool    packet_ready;                 /* 数据包就绪标志 (1: 新包待读取) */
} USART_t;

/* 兼容外部全局变量 */
extern USART_t myusart;
extern uint16_t Compare;    /* 开环速度占空比 (0 ~ 1000) */
extern bool Font;           /* 开环方向: 0 为正转 (+), 1 为反转 (-) */

/**
 * @brief 串口硬件与中断初始化
 */
void USART_Init(void);

/**
 * @brief 发送单个字节 (阻塞式)
 * @param ch 待发送字节
 */
void USART_Send_Byte(uint8_t ch);

/**
 * @brief 发送指定长度的数据缓冲区 (阻塞式)
 * @param buf 数据缓冲区首地址
 * @param len 发送字节数
 */
void USART_Send_Buff(const uint8_t *buf, uint16_t len);

/**
 * @brief 发送以 '\0' 结尾的字符串
 * @param str 待发送字符串
 */
void USART_Send_String(const char *str);

/**
 * @brief 发送字符串 (兼容别名)
 * @param str 待发送字符串
 */
void USART_Send_Str(char *str);

/**
 * @brief 发送定长 5 字节二进制 HEX 数据包: [0xA5] [D1] [D2] [SUM] [0x5A]
 * @param d1 数据1
 * @param d2 数据2
 */
void USART_SendPacket_Fixed(uint8_t d1, uint8_t d2);

/**
 * @brief 发送变长二进制 HEX 数据包: [0xA5] [LEN] [DATA_0 ... DATA_LEN-1] [SUM] [0x5A]
 * @param data 数据载荷指针 (len=0 时可为 NULL)
 * @param len  数据载荷长度 (0 ~ USART_PACKET_MAX)
 */
void USART_SendPacket_Var(const uint8_t *data, uint8_t len);

/**
 * @brief 查询是否有未读取的二进制 HEX 数据包
 * @return true: 有就绪数据包; false: 无
 */
bool USART_IsPacketReady(void);

/**
 * @brief 获取二进制 HEX 数据包有效载荷
 * @param out_data 输出数据缓冲区 (传入 NULL 时仅读取长度)
 * @param out_len  输出数据长度指针 (可为 NULL)
 * @return uint8_t 实际获取的数据载荷长度 (无就绪包时返回 0)
 * @note  调用后自动清除就绪标志
 */
uint8_t USART_GetPacket(uint8_t *out_data, uint8_t *out_len);

/**
 * @brief 针对定长数据包 (2 字节 Payload) 的快捷读取接口
 * @param d1 输出参数: 数据1
 * @param d2 输出参数: 数据2
 * @return true: 成功读取定长包; false: 无就绪包或载荷长度不为 2
 * @note  调用后自动清除就绪标志
 */
bool USART_GetFixedPacket(uint8_t *d1, uint8_t *d2);

/**
 * @brief 手动清除当前二进制 HEX 数据包就绪标志
 */
void USART_ClearPacket(void);

/**
 * @brief ASCII 文本指令帧处理函数 (在 main 主循环中非阻塞调用)
 */
void USART_ProcessFrame(void);

/**
 * @brief ASCII 文本指令语义解析与派发函数
 */
void USART_Data_Analyze(void);

/**
 * @brief 周期性轮询并通过串口上报电机状态 (PWM/RPM/循迹状态)
 * @param period_ms 上报周期 (单位: ms)
 */
void USART_Poll_MotorStatus(uint32_t period_ms);

/**
 * @brief 获取自上电以来 UART 实际接收到的总字节数 (RX 链路诊断)
 * @return uint32_t 累计接收字节数
 */
uint32_t USART_GetRxTotal(void);

#endif /* __USART_H */
