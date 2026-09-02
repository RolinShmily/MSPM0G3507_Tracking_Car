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
 *    its contributors may be scientific products derived
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
#include "BSP/PWM.h"
#include "BSP/Tick.h"
#include "BSP/Key.h"

int main(void)
{
    /* 1. 初始化系统外设 (时钟、GPIO、SysTick、PWM_LED 定时器等) */
    SYSCFG_DL_init();

    /* 2. 初始化 PWM 模块 */
    PWM_Init();

    while (1) {
        /* 
         * 3. 1秒内逐渐灭 (PB27 为低电平点亮，比较值从 0 增加到 1000，占空比渐暗)
         * 共 100 个阶梯，每阶梯延时 10ms，总耗时 100 * 10ms = 1000ms = 1s
         */
        for (uint32_t i = 0; i <= 100; i++) {
            PWM_Set_CompareValue(i * 10);
            delay_ms(10);
        }

        /* 
         * 4. 另1秒内逐渐亮 (比较值从 1000 减少到 0，占空比渐亮)
         * 共 100 个阶梯，每阶梯延时 10ms，总耗时 100 * 10ms = 1000ms = 1s
         */
        for (int32_t i = 100; i >= 0; i--) {
            PWM_Set_CompareValue(i * 10);
            delay_ms(10);
        }
    }
}
