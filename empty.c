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
#include "BSP/OLED.h"
#include "BSP/Motor.h"
#include "BSP/Key.h"
#include "BSP/Tick.h"

/**
 * @brief 在 OLED 屏幕上分行刷新显示电机的 PWM 占空比、转速档位与运行方向
 * @param gear 当前速度档位 (0 ~ 10)
 * @param dir  当前运行方向 (+1: 正转, -1: 反转)
 */
static void OLED_ShowMotorStatus(int gear, int dir)
{
    int duty = gear * 10;                /* 占空比 0 ~ 100% */
    int speed = dir * (gear * 100);       /* 带符号速度值 -1000 ~ +1000 */

    OLED_Clear();

    /* 第一行：PWM 占空比 (Y=0) */
    OLED_ShowString(0, 0, "占空比:", OLED_8X16);
    OLED_ShowNum(56, 0, duty, 3, OLED_8X16);
    OLED_ShowString(80, 0, "%", OLED_8X16);

    /* 第二行：转速档位与具体速度 (Y=16) */
    OLED_ShowString(0, 16, "转速:", OLED_8X16);
    OLED_ShowNum(48, 16, gear, 2, OLED_8X16);
    OLED_ShowString(64, 16, "档", OLED_8X16);
    OLED_ShowSignedNum(88, 16, speed, 4, OLED_8X16);

    /* 第三行：运行方向 (Y=32) */
    OLED_ShowString(0, 32, "方向:", OLED_8X16);
    if (dir > 0) {
        OLED_ShowString(48, 32, "正转", OLED_8X16);
    } else {
        OLED_ShowString(48, 32, "反转", OLED_8X16);
    }

    /* 刷新显存到 OLED 屏幕显示 */
    OLED_Update();
}

int main(void)
{
    /* 1. 初始化系统时钟、电源、GPIO、SysTick 及定时器外设 */
    SYSCFG_DL_init();

    /* 2. 初始化 OLED 屏幕 */
    OLED_Init();

    /* 3. 初始化电机驱动 (TIMG8 PWM 输出启动，初始静止) */
    Motor_Init();

    /* 
     * 4. 上电首先显示 2 秒图像 (居中展示 64x64 SrP Logo)
     */
    OLED_Clear();
    OLED_ShowImage(32, 0, 64, 64, gImage_SrP_64x64);
    OLED_Update();
    delay_ms(2000);

    /* 5. 初始电机状态变量 */
    int gear = 0;   /* 当前档位 (0 ~ 10 档，每档 10% 占空比) */
    int dir = 1;    /* 运行方向 (+1: 正转, -1: 反转) */

    /* 6. 显示 2s 图像后，切换为分行显示电机状态 UI */
    OLED_ShowMotorStatus(gear, dir);

    while (1) {
        /* 7. 获取滴答定时器状态机按键事件 (非阻塞，读后即清) */
        KeyEvent_t key_evt = Key_GetEvent();

        if (key_evt == KEY_EVENT_SHORT_PRESS) {
            /* 
             * 短按事件：分阶加速
             * 档位 +1，若超过 10 档 (溢出) 则归零
             */
            gear++;
            if (gear > MOTOR_GEAR_MAX) {
                gear = 0;
            }
            /* 设置电机速度并实时刷新 OLED 显示 */
            Motor_SetSpeed(dir * (gear * 100));
            OLED_ShowMotorStatus(gear, dir);
        } 
        else if (key_evt == KEY_EVENT_LONG_PRESS) {
            /* 
             * 长按事件：PWM 输出与电机转向反向
             * 方向变量直接取负号 (dir = -dir)
             */
            dir = -dir;
            Motor_SetSpeed(dir * (gear * 100));
            OLED_ShowMotorStatus(gear, dir);
        }

        delay_ms(10);
    }
}