#ifndef __LED_H
#define __LED_H

#include "ti_msp_dl_config.h"

/**
 * @brief 打开 LED
 */
void LED_ON(void);

/**
 * @brief 关闭 LED
 */
void LED_OFF(void);

/**
 * @brief 翻转 LED 状态
 */
void LED_TOGGLE(void);

/**
 * @brief 翻转 LED 状态并延时指定毫秒数
 * @param ms 翻转后延时的毫秒数
 */
void LED_BLINK(uint32_t ms);

#endif /* __LED_H */
