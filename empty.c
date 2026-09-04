/*
 * Copyright (c) 2021, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "ti_msp_dl_config.h"
#include "BSP/Key.h"
#include "BSP/Tick.h"
#include "BSP/UART.h"
#include <stdio.h>

/* UART RX 接收环形缓冲（UART_Init 使能了 RX 中断，必须提供 ISR 覆盖弱定义） */
#define UART_RX_BUF_SIZE   128
static volatile uint8_t  g_uart_rx_buf[UART_RX_BUF_SIZE];
static volatile uint16_t g_uart_rx_head = 0;
static volatile uint16_t g_uart_rx_tail = 0;

/**
 * @brief UART0 (UART_Debug) 中断服务函数：收到一字节写入环形缓冲
 */
void UART_Debug_INST_IRQHandler(void)
{
    switch (DL_UART_Main_getPendingInterrupt(UART_Debug_INST)) {
        case DL_UART_MAIN_IIDX_RX: {
            uint8_t rx = DL_UART_Main_receiveData(UART_Debug_INST);
            uint16_t next = (uint16_t)((g_uart_rx_head + 1) % UART_RX_BUF_SIZE);
            if (next != g_uart_rx_tail) {
                g_uart_rx_buf[g_uart_rx_head] = rx;
                g_uart_rx_head = next;
            }
            break;
        }
        default:
            break;
    }
}

int main(void)
{
    /* 1. 初始化系统时钟/GPIO/UART0 等外设 (SysConfig 生成) */
    SYSCFG_DL_init();

    /* 2. 初始化 UART_Debug (UART0)：使能 RX 中断 */
    UART_Init();

    /* 3. 定义按键调控的全局变量 */
    int Compare = 0;      /* 占空比/速度变量：短按每次 +100，>1000 归 0 */
    int Font = 0;         /* 方向变量：0=正转(Forward), 1=反转(Backward) */

    char txbuff[64];      /* 发送缓冲区 */

    /* 4. 上电欢迎信息 */
    UART_Send_Str("=== MSPM0G3507 UART Debug Start ===\r\n");

    while (1)
    {
        /* 5. 获取按键事件 */
        KeyEvent_t key_evt = Key_GetEvent();

        if (key_evt == KEY_EVENT_SHORT_PRESS)
        {
            /* 短按：变量每次 +100，溢出归 0 */
            Compare += 100;
            if (Compare > 1000) {
                Compare = 0;
            }
            sprintf((char *)txbuff, "Compare:%d\r\n", Compare);
            UART_Send_Str((char *)txbuff);
        }
        else if (key_evt == KEY_EVENT_LONG_PRESS)
        {
            /* 长按：方向取反 */
            Font = !Font;
            if (Font == 0) {
                UART_Send_Str("Font:Forward");
            }
            else if (Font == 1) {
                UART_Send_Str("Font:Backward\r\n");
            }
        }

        /* 6. 延时，避免空转 */
        delay_ms(10);
    }
}
