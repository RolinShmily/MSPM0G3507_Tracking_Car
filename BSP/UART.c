#include "UART.h"

/**
 * @brief UART_Debug (UART0) 串口初始化函数
 */
void UART_Init(void)
{
    /* SysConfig 已在 SYSCFG_DL_init() 中完成 UART0 波特率/GPIO 等硬件配置。
       此处仅使能 UART0 接收中断，供 RX 数据接收使用。 */
    NVIC_EnableIRQ(UART_Debug_INST_INT_IRQN);
}

/**
 * @brief 串口发送单个字符
 * @param ch 要发送的字符
 */
void UART_Send_Byte(char ch)
{
    /* 当串口忙的时候等待，不忙的时候再发送传进来的字符 */
    while (DL_UART_isBusy(UART_Debug_INST) == true) {
        /* 阻塞等待 UART 空闲 */
    }
    /* 发送单个字符 */
    DL_UART_Main_transmitData(UART_Debug_INST, (uint8_t)ch);
}

/**
 * @brief 串口发送字符串
 * @param str 以 '\0' 结尾的字符串首地址
 */
void UART_Send_Str(char *str)
{
    /* 当前字符串地址不在结尾 并且 字符串首地址不为空 */
    while (*str != '\0')
    {
        /* 发送字符串首地址中的字符，并且在发送完成之后首地址自增 */
        UART_Send_Byte(*str++);
    }
}
