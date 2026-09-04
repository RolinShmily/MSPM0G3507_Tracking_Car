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
#include "BSP/OLED.h"
#include "BSP/Gray.h"
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    /* 1. 初始化系统时钟/GPIO/TIMG8/UART0/I2C0 等外设 (SysConfig 自动生成) */
    SYSCFG_DL_init();

    /* 2. 初始化各模块: 电机驱动、串口通信、OLED显示屏、灰度传感器 */
    Motor_Init();
    UART_Init();
    OLED_Init();
    Gray_Init();

    /* 3. 串口打印上电提示 */
    UART_Send_Str("\r\n=== MSPM0G3507 ===\r\n");

    /* 4. OLED 初始静态界面显示 */
    OLED_Clear();
    OLED_ShowString(0, 0,  "Gray Sensor 8CH", OLED_8X16);
    OLED_ShowString(0, 18, "1 2 3 4 5 6 7 8", OLED_8X16);
    OLED_Update();

    char gray_str[16];
    char display_str[32];

    while (1)
    {
        /* 5. 实时获取 8 路灰度传感器状态 (如 "01010101") */
        Gray_GetStatusString(gray_str);

        /* 6. 在 OLED 屏幕上实时刷新传感器状态 (与表头 1~8 字符对齐显示) */
        sprintf(display_str, "%c %c %c %c %c %c %c %c",
                gray_str[0], gray_str[1], gray_str[2], gray_str[3],
                gray_str[4], gray_str[5], gray_str[6], gray_str[7]);
        OLED_ShowString(0, 36, display_str, OLED_8X16);
        OLED_Update();


        /* 8. 主循环刷新延时 20ms (刷新率约 50Hz) */
        delay_ms(20);
    }
}
