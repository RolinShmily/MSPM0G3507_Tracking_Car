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
#include "BSP/Tick.h"
#include "BSP/UART.h"
#include "BSP/Motor.h"
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    /* 1. 初始化系统时钟/GPIO/TIMG8/UART0 等外设 (SysConfig 自动生成) */
    SYSCFG_DL_init();

    /* 2. 初始化电机 (TIMG8 PWM) 与 UART (使能 RX 中断) */
    Motor_Init();
    UART_Init();

    /* 3. 上电通过串口打印就绪提示及控制指令格式 */
    UART_Send_Str("\r\n=========================================\r\n");
    UART_Send_Str(" MSPM0G3507 Motor UART Control Ready\r\n");
    UART_Send_Str(" Supported Commands:\r\n");
    UART_Send_Str("  1. Compare:<0-1000>  (e.g. Compare:500)\r\n");
    UART_Send_Str("  2. Forward           (Set direction forward)\r\n");
    UART_Send_Str("  3. Backward          (Set direction backward)\r\n");
    UART_Send_Str("  4. Signed Speed      (e.g. 600, -400, 0)\r\n");
    UART_Send_Str("=========================================\r\n\r\n");

    while (1)
    {
        /* 4. 周期性轮询并通过串口输出当前电机运行状态 (500ms 间隔，非阻塞) */
        UART_Poll_MotorStatus(500);

        /* 5. 主循环延时 1ms，稳定运行节拍 */
        delay_ms(1);
    }
}
