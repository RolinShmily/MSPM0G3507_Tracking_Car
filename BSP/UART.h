#ifndef __UART_H
#define __UART_H

#include "ti_msp_dl_config.h"
#include <stdint.h>
#include <stdbool.h>

/* 串口通信结构体（对齐参考例程） */
typedef struct {
    uint8_t rxbuff[100];    /* 接收缓冲区 */
    uint8_t rxcount;        /* 接收字符计数器 */
    uint8_t rxover;         /* 接收完成标志位 (1: 一帧接收完毕) */
    uint8_t txbuff[100];    /* 发送缓冲区 */
} USART_t;

/* 全局串口结构体与电机状态变量 */
extern USART_t myusart;
extern uint16_t Compare;    /* 速度占空比 (0 ~ 1000) */
extern bool Font;           /* 方向: 0 为正转 (Forward), 1 为反转 (Backward) */

/**
 * @brief UART_Debug (UART0) 串口初始化函数
 */
void UART_Init(void);

/**
 * @brief 串口发送单个字符（底层标准阻塞发送）
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
 * @brief 串口接收数据解析函数（对齐截图 Data_Anylize）
 * @note  在一帧接收完成（收到 \r\n）时调用，解析 Compare 与 Forward/Backward，并同步更新电机
 */
void Data_Anylize(void);

/**
 * @brief 串口轮询发送电机状态（左速度、右速度、方向），合成一条字符串周期发出
 * @param period_ms 发送周期，单位毫秒（建议 500ms，避免 9600 波特率通道堵塞）
 */
void UART_Poll_MotorStatus(uint32_t period_ms);

#endif /* __UART_H */
