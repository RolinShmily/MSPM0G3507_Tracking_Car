#include "LED.h"
#include "Tick.h"

/**
 * @brief 打开 LED (低电平点亮，PB27 输出低电平)
 */
void LED_ON(void)
{
    DL_GPIO_clearPins(LED_PORT, LED_PIN_27_PIN);
}

/**
 * @brief 关闭 LED (高电平熄灭，PB27 输出高电平)
 */
void LED_OFF(void)
{
    DL_GPIO_setPins(LED_PORT, LED_PIN_27_PIN);
}

/**
 * @brief 翻转 LED 状态
 */
void LED_TOGGLE(void)
{
    DL_GPIO_togglePins(LED_PORT, LED_PIN_27_PIN);
}

/**
 * @brief 翻转 LED 状态并延时指定的毫秒数
 * @param ms 延时毫秒数
 */
void LED_BLINK(uint32_t ms)
{
    /* 1. 执行 LED 引脚翻转 */
    DL_GPIO_togglePins(LED_PORT, LED_PIN_27_PIN);
    /* 2. 若 ms > 0 则调用 Tick 模块进行延时 */
    if (ms > 0) {
        delay_ms(ms);
    }
}
