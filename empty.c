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
 * *  Neither the name of Texas Instruments Incorporated nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
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
#include "BSP/Encoder.h"
#include "BSP/SpeedCtrl.h"
#include "BSP/OLED.h"
#include "BSP/Gray.h"
#include "BSP/Track.h"
#include "BSP/Key.h"
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    /* 1. 初始化系统时钟/GPIO/TIMG8/UART0/I2C0 等外设 (SysConfig 自动生成) */
    SYSCFG_DL_init();

    /* 2. 初始化各模块: 电机驱动、串口通信、OLED 显示、灰度传感器、编码器、按键 */
    Motor_Init();
    UART_Init();
    OLED_Init();
    Gray_Init();
    Encoder_Init();
    SpeedCtrl_Init();
    Track_Init();
    Key_Init();

    /* 4. OLED 初始化静态界面显示 */
    OLED_Clear();
    OLED_ShowString(0, 0,  "Gray Sensor 8CH", OLED_8X16);
    OLED_ShowString(0, 18, "8 7 6 5 4 3 2 1", OLED_8X16);
    OLED_Update();

    char gray_str[16];
    char display_str[32];
    uint32_t last_oled_tick = 0;

    while (1)
    {
        uint32_t now = get_ticks();

        /* 5. 解耦 OLED 屏幕刷新: 每 200ms 刷新一次，彻底避免 I2C 阻塞主循环影响串口响应 */
        if (now - last_oled_tick >= 200) {
            last_oled_tick = now;

            /* 实时读取 8 路灰度传感器状态 (形如 "01010101") */
            Gray_GetStatusString(gray_str);

            /* 在 OLED 屏幕实时刷新传感器状态 (探头 8~1 字符间隔显示) */
            sprintf(display_str, "%c %c %c %c %c %c %c %c",
                    gray_str[0], gray_str[1], gray_str[2], gray_str[3],
                    gray_str[4], gray_str[5], gray_str[6], gray_str[7]);
            OLED_ShowString(0, 36, display_str, OLED_8X16);

            /* 显示左右轮 1s 窗口转速 (前缀 +/- 表示方向) */
            sprintf(display_str, "L%+5d R%+5d RPM",
                    Encoder_GetLRPM(), Encoder_GetRRPM());
            OLED_ShowString(0, 54, display_str, OLED_6X8);

            OLED_Update();
        }

        /* 6. 按键检测: 切换循迹与闭环启停 (Scheme B) */
        KeyEvent_t key_evt = Key_GetEvent();
        if (key_evt != KEY_EVENT_NONE) {
            if (Track_IsEnabled()) {
                Track_Enable(0);
                SpeedCtrl_Enable(0);
                UART_Send_Str("[KEY] Track Stopped\r\n");
            } else {
                SpeedCtrl_Enable(1);
                Track_Enable(1);
                UART_Send_Str("[KEY] Track Started\r\n");
            }
        }

        /* 7. 帧处理: 回显+解析串口指令并控制电机 (主循环即时响应, 零中断) */
        UART_ProcessFrame();

        /* 8. 周期性查询并通过串口上报电机当前状态 (200ms 一次) */
        UART_Poll_MotorStatus(200);

        /* 9. 主循环快速轮询延时 1ms, 保证串口指令即时响应 */
        delay_ms(1);
    }
}
