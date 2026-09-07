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
 * AND ANY EXPRESS OR IMPLIED WARRANTIES ARE DISCLAIMED. IN NO EVENT SHALL
 * THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "ti_msp_dl_config.h"
#include "BSP/Tick.h"
#include "BSP/UART.h"
#include "BSP/Motor.h"
#include "BSP/Encoder.h"
#include "BSP/SpeedCtrl.h"
#include "BSP/OLED.h"
#include "BSP/Gray.h"
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    /* 1. 初始化系统时钟/GPIO/TIMG8/UART0/I2C0 等外设 (SysConfig 自动生成) */
    SYSCFG_DL_init();

    /* 2. 初始化各模块: 电机驱动、串口通信、OLED 显示、灰度传感器、编码器 */
    Motor_Init();
    UART_Init();
    OLED_Init();
    Gray_Init();
    Encoder_Init();
    SpeedCtrl_Init();

    /* 3. 上电通过串口打印帮助信息, 提示指令格式说明 */
    UART_Send_Str("\r\n=========================================\r\n");
    UART_Send_Str(" MSPM0G3507 Motor UART Control Ready\r\n");
    UART_Send_Str(" Simplified Commands:\r\n");
    UART_Send_Str("   Sp<value> : Set Speed (e.g. Sp100, Sp500)\r\n");
    UART_Send_Str("   +         : Forward Direction\r\n");
    UART_Send_Str("   -         : Backward Direction\r\n");
    UART_Send_Str("=========================================\r\n\r\n");

    /* 4. OLED 初始化静态界面显示 */
    OLED_Clear();
    OLED_ShowString(0, 0,  "Gray Sensor 8CH", OLED_8X16);
    OLED_ShowString(0, 18, "1 2 3 4 5 6 7 8", OLED_8X16);
    OLED_Update();

    char gray_str[16];
    char display_str[32];

    while (1)
    {
        /* 5. 实时读取 8 路灰度传感器状态 (形如 "01010101") */
        Gray_GetStatusString(gray_str);

        /* 6. 在 OLED 屏幕实时刷新传感器状态 (探头 1~8 字符间隔显示) */
        sprintf(display_str, "%c %c %c %c %c %c %c %c",
                gray_str[0], gray_str[1], gray_str[2], gray_str[3],
                gray_str[4], gray_str[5], gray_str[6], gray_str[7]);
        OLED_ShowString(0, 36, display_str, OLED_8X16);

        /* 7. 显示左右轮 1s 窗口转速 (前缀 +/- 表示方向) */
        sprintf(display_str, "L%+5d R%+5d RPM",
                Encoder_GetLRPM(), Encoder_GetRRPM());
        OLED_ShowString(0, 54, display_str, OLED_6X8);

        OLED_Update();

        /* 8. 帧处理: 回显+解析串口指令并控制电机 (主循环上下文, 非中断) */
        UART_ProcessFrame();

        /* 9. 周期性查询并通过串口上报电机当前状态 (500ms 一次) */
        UART_Poll_MotorStatus(500);

        /* 10. 主循环刷新间隔 20ms (刷新率约 50Hz) */
        delay_ms(20);
    }
}
