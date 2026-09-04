#ifndef __UART_H
#define __UART_H

#include "ti_msp_dl_config.h"
#include <stdint.h>

/* ==================== 回显状态机相关定义 ==================== */

/* 接收环形缓冲大小 */
#define UART_RX_BUF_SIZE    128

/* 回显状态机状态 */
typedef enum {
    UART_ECHO_IDLE = 0,     /* 空闲：等待数据到达 */
    UART_ECHO_RECEIVING,    /* 接收中：正在缓冲数据 */
    UART_ECHO_SENDING       /* 发送中：将缓冲回显给上位机 */
} UART_EchoState_t;

/* 串口回显结构体（状态机实例） */
typedef struct {
    volatile uint8_t  rx_buf[UART_RX_BUF_SIZE];  /* RX 环形缓冲 */
    volatile uint16_t rx_head;                   /* ISR 写指针 */
    volatile uint16_t rx_tail;                   /* 主循环读指针 */
    UART_EchoState_t  echo_state;                /* 回显状态机状态 */
} UART_t;

/**
 * @brief UART_Debug (UART0) 串口初始化函数
 * @note  硬件寄存器已由 SysConfig 生成的 SYSCFG_DL_init() 完成配置，
 *        本函数额外使能 UART0 中断并初始化回显状态机。
 * @param None
 * @return None
 */
void UART_Init(void);

/**
 * @brief 串口发送单个字符
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
 * @brief 串口回显处理函数（状态机）
 * @note  在 main 主循环中调用。若 RX 缓冲中有数据，
 *        则逐个读取并原样回显给上位机，实现 Windows 串口助手发送→MCU 回显。
 */
void UART_Echo_Process(void);

#endif /* __UART_H */
