#ifndef __GRAY_H
#define __GRAY_H

#include "ti_msp_dl_config.h"
#include <stdint.h>

/**
 * @brief 8 路灰度传感器模块初始化
 */
void Gray_Init(void);

/**
 * @brief 读取指定通道的灰度传感器电平状态
 * @param ch 通道编号: 1 ~ 8 (对应 OUT1 ~ OUT8)
 * @return 0 (低电平) 或 1 (高电平)
 */
uint8_t Gray_ReadChannel(uint8_t ch);

/**
 * @brief 获取 8 路灰度传感器状态并填充为 8 位字符格式字符串 (车头视角从左到右: OUT8 ~ OUT1)
 * @param out_str 存储字符串的目标缓冲区 (长度需至少 9 字节，含 '\0')
 */
void Gray_GetStatusString(char *out_str);

/**
 * @brief 读取 8 路灰度传感器状态并组合为一个字节 (bit 0 对应 OUT1, bit 7 对应 OUT8)
 * @return uint8_t 8 通道按位组合值
 */
uint8_t Gray_ReadByte(void);

#endif /* __GRAY_H */
