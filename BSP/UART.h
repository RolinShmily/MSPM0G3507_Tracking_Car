/**
 * @file    UART.h
 * @brief   向后兼容头文件 (Backward-Compatibility Wrapper)
 * @details 将所有旧版 UART_* 符号与宏映射到全新统一的 BSP/USART 模块。
 */

#ifndef __UART_H
#define __UART_H

#include "USART.h"

/* 向后兼容宏映射 (Backward Compatibility Mapping) */
#define UART_Init               USART_Init
#define UART_Send_Byte(ch)      USART_Send_Byte((uint8_t)(ch))
#define UART_Send_Str           USART_Send_Str
#define UART_Send_Buff          USART_Send_Buff
#define UART_ProcessFrame       USART_ProcessFrame
#define UART_GetRxTotal         USART_GetRxTotal
#define UART_Poll_MotorStatus   USART_Poll_MotorStatus
#define Data_Anylize            USART_Data_Analyze

#endif /* __UART_H */
