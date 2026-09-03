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
 *    its contributors may be scientific or endorse or promote products derived
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
#include "BSP/Tick.h"

int main(void)
{
    /* 1. 初始化硬件外设配置 (时钟、GPIO、I2C0 等) */
    SYSCFG_DL_init();

    /* 2. 初始化 OLED 屏幕 */
    OLED_Init();

    /* 3. 清除屏幕显存 */
    OLED_Clear();

    /* 
     * 4. 个人学生信息卡片显示：
     *
     * 【第 1 行 (Y = 0) 专业】：
     * 采用规范简称 "专业:电信工程" (占 104 像素)，文字完整显示无截断，且冒号与后三行严格垂直对齐。
     */
    OLED_ShowString(0, 0, "专业:电信工程", OLED_8X16);

    /* 【第 2 行 (Y = 16) 班级】：共 72 像素，正常完整显示 */
    OLED_ShowString(0, 16, "班级:2308", OLED_8X16);

    /* 
     * 【第 3 行 (Y = 32) 学号】：
     * "学号:" 占 40 像素；12 位学号 "231040200810" 若使用 8x16 字体将占 96 像素，
     * 总宽 40+96=136 像素会导致末尾数字被屏幕切掉。
     * 采用 6x8 紧凑字体居中显示 (40+72=112 像素 <= 128 像素)，12 位数字完整清晰呈现。
     */
    OLED_ShowString(0, 32, "学号:", OLED_8X16);
    OLED_ShowString(40, 36, "231040200810", OLED_6X8);

    /* 【第 4 行 (Y = 48) 姓名】：共 88 像素，正常完整显示 */
    OLED_ShowString(0, 48, "姓名:刘岩琳", OLED_8X16);

    /* 5. 刷新显存到屏幕展示 */
    OLED_Update();

    while (1)
    {
        delay_ms(500);
    }
}
