#include "Gray.h"

void Gray_Init(void)
{
    /* 引脚由 SYSCFG_DL_init() 中的 SYSCFG_DL_GPIO_init() 统一初始化为数字输入模式 */
}

uint8_t Gray_ReadChannel(uint8_t ch)
{
    switch (ch) {
        case 1: return (DL_GPIO_readPins(GRAYA_PORT, GRAYA_PIN_07_OUT1_PIN) != 0) ? 1 : 0;
        case 2: return (DL_GPIO_readPins(GRAYA_PORT, GRAYA_PIN_08_OUT2_PIN) != 0) ? 1 : 0;
        case 3: return (DL_GPIO_readPins(GRAYB_PORT, GRAYB_PIN_19_OUT3_PIN) != 0) ? 1 : 0;
        case 4: return (DL_GPIO_readPins(GRAYA_PORT, GRAYA_PIN_18_OUT4_PIN) != 0) ? 1 : 0;
        case 5: return (DL_GPIO_readPins(GRAYB_PORT, GRAYB_PIN_05_OUT5_PIN) != 0) ? 1 : 0;
        case 6: return (DL_GPIO_readPins(GRAYA_PORT, GRAYA_PIN_13_OUT6_PIN) != 0) ? 1 : 0;
        case 7: return (DL_GPIO_readPins(GRAYA_PORT, GRAYA_PIN_12_OUT7_PIN) != 0) ? 1 : 0;
        case 8: return (DL_GPIO_readPins(GRAYA_PORT, GRAYA_PIN_22_OUT8_PIN) != 0) ? 1 : 0;
        default: return 0;
    }
}

void Gray_GetStatusString(char *out_str)
{
    if (out_str == 0) return;
    out_str[0] = (DL_GPIO_readPins(GRAYA_PORT, GRAYA_PIN_07_OUT1_PIN) != 0) ? '1' : '0';
    out_str[1] = (DL_GPIO_readPins(GRAYA_PORT, GRAYA_PIN_08_OUT2_PIN) != 0) ? '1' : '0';
    out_str[2] = (DL_GPIO_readPins(GRAYB_PORT, GRAYB_PIN_19_OUT3_PIN) != 0) ? '1' : '0';
    out_str[3] = (DL_GPIO_readPins(GRAYA_PORT, GRAYA_PIN_18_OUT4_PIN) != 0) ? '1' : '0';
    out_str[4] = (DL_GPIO_readPins(GRAYB_PORT, GRAYB_PIN_05_OUT5_PIN) != 0) ? '1' : '0';
    out_str[5] = (DL_GPIO_readPins(GRAYA_PORT, GRAYA_PIN_13_OUT6_PIN) != 0) ? '1' : '0';
    out_str[6] = (DL_GPIO_readPins(GRAYA_PORT, GRAYA_PIN_12_OUT7_PIN) != 0) ? '1' : '0';
    out_str[7] = (DL_GPIO_readPins(GRAYA_PORT, GRAYA_PIN_22_OUT8_PIN) != 0) ? '1' : '0';
    out_str[8] = '\0';
}

uint8_t Gray_ReadByte(void)
{
    uint8_t data = 0;
    for (uint8_t i = 1; i <= 8; i++) {
        if (Gray_ReadChannel(i)) {
            data |= (uint8_t)(1 << (i - 1));
        }
    }
    return data;
}
