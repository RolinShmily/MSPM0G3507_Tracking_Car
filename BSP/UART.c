#include "UART.h"

/* 回显状态机全局实例 */
static UART_t g_uart = {
    .rx_buf     = {0},
    .rx_head    = 0,
    .rx_tail    = 0,
    .echo_state = UART_ECHO_IDLE,
};

/**
 * @brief UART0 (UART_Debug) 中断服务函数：收到一字节写入环形缓冲
 */
void UART_Debug_INST_IRQHandler(void)
{
    switch (DL_UART_Main_getPendingInterrupt(UART_Debug_INST)) {
        case DL_UART_MAIN_IIDX_RX: {
            uint8_t rx = DL_UART_Main_receiveData(UART_Debug_INST);
            uint16_t next = (uint16_t)((g_uart.rx_head + 1) % UART_RX_BUF_SIZE);
            if (next != g_uart.rx_tail) {       /* 环形缓冲未满则写入 */
                g_uart.rx_buf[g_uart.rx_head] = rx;
                g_uart.rx_head = next;
            }
            break;
        }
        default:
            break;
    }
}

/**
 * @brief UART_Debug (UART0) 串口初始化函数
 */
void UART_Init(void)
{
    /* SysConfig 已在 SYSCFG_DL_init() 中完成 UART0 波特率/GPIO 等硬件配置。
       此处仅使能 UART0 接收中断，供 RX 数据接收使用。 */
    NVIC_EnableIRQ(UART_Debug_INST_INT_IRQN);

    /* 初始化回显状态机 */
    g_uart.rx_head    = 0;
    g_uart.rx_tail    = 0;
    g_uart.echo_state = UART_ECHO_IDLE;
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

/**
 * @brief 串口发送指定长度的缓冲区
 * @param str   缓冲区首地址
 * @param lenth 要发送的字节数
 */
void UART_Send_Buff(uint8_t *str, uint8_t lenth)
{
    /* 遍历缓冲区，逐字节发送数据 */
    for (uint8_t i = 0; i < lenth; i++)
    {
        UART_Send_Byte(str[i]);
    }
}

/**
 * @brief 串口回显处理函数（状态机）
 * @note  在 main 主循环中调用。若 RX 环形缓冲中有数据，
 *        则逐个读取并原样回显给上位机。
 */
void UART_Echo_Process(void)
{
    switch (g_uart.echo_state)
    {
        case UART_ECHO_IDLE:
            /* 有数据到达则进入接收状态 */
            if (g_uart.rx_head != g_uart.rx_tail) {
                g_uart.echo_state = UART_ECHO_RECEIVING;
            }
            break;

        case UART_ECHO_RECEIVING:
            /* 读取缓冲中的一字节并回显 */
            if (g_uart.rx_head != g_uart.rx_tail)
            {
                uint8_t c = g_uart.rx_buf[g_uart.rx_tail];
                g_uart.rx_tail = (uint16_t)((g_uart.rx_tail + 1) % UART_RX_BUF_SIZE);

                /* 逐字节回显给上位机 */
                UART_Send_Byte((char)c);
            }
            else
            {
                /* 缓冲读空，回到空闲状态 */
                g_uart.echo_state = UART_ECHO_IDLE;
            }
            break;

        case UART_ECHO_SENDING:
        default:
            g_uart.echo_state = UART_ECHO_IDLE;
            break;
    }
}
