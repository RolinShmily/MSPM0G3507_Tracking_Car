#ifndef __UART_H
#define __UART_H

#include "ti_msp_dl_config.h"

/**
 * @brief UART_Debug (UART0) 串口初始化函数
 * @note  硬件寄存器已由 SysConfig 生成的 SYSCFG_DL_init() 完成配置，
 *        本函数额外使能 UART0 中断（若需 RX 接收）。
 */
void UART_Init(void);

/**
 * @brief 串口发送单个字符
 * @param ch 要发送的字符
 * @note  当 UART 忙时阻塞等待，空闲后再发送。
 */
void UART_Send_Byte(char ch);

/**
 * @brief 串口发送字符串
 * @param str 以 '\0' 结尾的字符串首地址 (不能为空)
 * @note  逐字符调用 UART_Send_Byte 直至字符串结尾。
 */
void UART_Send_Str(char *str);

#endif /* __UART_H */
