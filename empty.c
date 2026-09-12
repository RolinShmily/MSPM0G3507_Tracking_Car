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
#include "BSP/LED.h"
#include "BSP/Key.h"
#include "BSP/Tick.h"

/* PA18 按键中断标志位 (使用 volatile 确保中断与主循环间数据可见性) */
volatile uint8_t g_key18_flag = 0;

/**
 * @brief GPIOA 端口中断服务函数 (MSPM0 中 GPIOA 属于 Group 1 中断源)
 * @note  遵循标志位设计原则，中断内仅置标志位，不写入具体控制逻辑
 */
void GROUP1_IRQHandler(void)
{
    /* 查询当前产生中断的引脚，DL_GPIO_getPendingInterrupt 会返回对应的 IIDX 并清除标志 */
    switch (DL_GPIO_getPendingInterrupt(KEY1_PORT)) {
        case KEY1_PIN_18_IIDX:
            g_key18_flag = 1; /* 仅置位标志位 */
            break;
        default:
            break;
    }
}

int main(void)
{
    /* 1. 初始化系统时钟、电源、GPIO 与 SysTick */
    SYSCFG_DL_init();

    /* 2. 在 NVIC 中使能 GPIOA (Group 1) 外部中断 */
    NVIC_EnableIRQ(KEY1_INT_IRQN);

    while (1) {
        /* 3. 在主循环中检测按键中断标志位 */
        if (g_key18_flag) {
            /* 4. 软件消抖延时 20ms，滤除机械按键抖动产生的多次误触发 */
            delay_ms(20);

            /* 5. 清除中断标志位 */
            g_key18_flag = 0;

            /* 6. 确认有效触发后，执行 LED 状态切换 */
            LED_TOGGLE();
        }
    }
}
