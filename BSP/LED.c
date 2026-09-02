#include "LED.h"
#include "Tick.h"

/*
 * 注意：由于 SysConfig 中已删除 GPIO "LED" 定义并切换为 PWM 驱动，
 * 原基于 GPIO 寄存器的控制代码暂时注释。
 */

/*
void LED_ON(void)
{
    DL_GPIO_clearPins(LED_PORT, LED_PIN_27_PIN);
}

void LED_OFF(void)
{
    DL_GPIO_setPins(LED_PORT, LED_PIN_27_PIN);
}

void LED_TOGGLE(void)
{
    DL_GPIO_togglePins(LED_PORT, LED_PIN_27_PIN);
}

void LED_BLINK(uint32_t ms)
{
    DL_GPIO_togglePins(LED_PORT, LED_PIN_27_PIN);
    if (ms > 0) {
        delay_ms(ms);
    }
}
*/
