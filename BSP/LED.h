#ifndef __LED_H
#define __LED_H

#include "ti_msp_dl_config.h"

/*
 * 注意：由于 SysConfig 中已将 PB27 由普通 GPIO 切换为 PWM_LED 定时器输出引脚，
 * 原 GPIO 相关的宏定义 (LED_PORT, LED_PIN_27_PIN) 已被移除。
 * 此处相关 GPIO 操作接口已注释，如需控制 LED 亮度请使用 BSP/PWM.h 模块。
 */

/*
void LED_ON(void);
void LED_OFF(void);
void LED_TOGGLE(void);
void LED_BLINK(uint32_t ms);
*/

#endif /* __LED_H */
