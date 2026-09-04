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

int main(void)
{
    /* 1. 初始化系统时钟/GPIO/UART0 等外设 (SysConfig 生成) */
    SYSCFG_DL_init();

    /* 2. 初始化 UART_Debug (UART0)：使能 RX 中断并初始化回显状态机 */
    UART_Init();

    /* 3. 定义按键调控的全局变量 */
    int Compare = 0;      /* 占空比/速度变量：短按每次 +100，>1000 归 0 */
    int Font = 0;         /* 方向变量：0=正转(Forward), 1=反转(Backward) */

    char txbuff[64];      /* 发送缓冲区 */

    /* 4. 上电欢迎信息 */
    UART_Send_Str("=== MSPM0G3507 UART Debug Start ===\r\n");

    while (1)
    {
        /* 5. 串口回显：接收 Windows 串口助手数据并原样回显 */
        UART_Echo_Process();

    }
}
