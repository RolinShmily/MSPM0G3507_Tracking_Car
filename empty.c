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
#include "BSP/Motor.h"
#include "BSP/Key.h"
#include "BSP/Tick.h"

int main(void)
{
    /* 1. 初始化系统时钟、电源、GPIO、SysTick 及定时器外设 */
    SYSCFG_DL_init();

    /* 2. 初始化电机驱动 (TIMG8 PWM 启动，初始静止) */
    Motor_Init();

    int gear = 0;   /* 当前档位 (0 ~ 10 档，每档 10% 占空比) */
    int dir = 1;    /* 运行方向 (+1: 正转, -1: 反转) */

    while (1) {
        /* 3. 获取滴答定时器状态机按键事件 (非阻塞，读后即清) */
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
            /* 计算并设置带符号的速度值 */
            Motor_SetSpeed(dir * (gear * 100));
        } 
        else if (key_evt == KEY_EVENT_LONG_PRESS) {
            /* 
             * 长按事件：PWM 输出与电机转向反向
             * 方向变量直接取负号 (dir = -dir)
             */
            dir = -dir;
            Motor_SetSpeed(dir * (gear * 100));
        }
    }
}
