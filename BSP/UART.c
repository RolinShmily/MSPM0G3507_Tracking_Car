#include "UART.h"
#include "Motor.h"
#include "Tick.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

/* 全局串口结构体与全局变量定义 */
USART_t myusart = {
    .rxbuff  = {0},
    .rxcount = 0,
    .rxover  = 0,
    .txbuff  = {0},
};

uint16_t Compare = 0;   /* 初始速度为 0 */
bool Font = 0;          /* 初始方向为 0 (Forward 正转) */

static char* data_p;
static uint8_t msg[100];
static uint8_t RxState = 0;

/**
 * @brief UART0 (UART_Debug) 中断服务函数
 * @note  逐字节接收字符；收到回车/换行表示一帧结束，
 *        添加 '\0' 结束符、置 rxover=1、原样回显缓冲区、执行 Data_Anylize 解析并重置计数器
 */
void UART_Debug_INST_IRQHandler(void)
{
    switch (DL_UART_Main_getPendingInterrupt(UART_Debug_INST)) {
        case DL_UART_MAIN_IIDX_RX: {
            uint8_t rx = DL_UART_Main_receiveData(UART_Debug_INST);

            /* 收到 \r 或 \n 判定为一帧接收完成 */
            if (rx == '\r' || rx == '\n')
            {
                if (myusart.rxcount > 0)
                {
                    /* 收到换行，一帧接收完成，添加字符串结束符 */
                    myusart.rxbuff[myusart.rxcount] = '\0';
                    myusart.rxover = 1;

                    /* 回显所收到的缓冲区内容 */
                    UART_Send_Buff(myusart.rxbuff, myusart.rxcount);
                    UART_Send_Str("\r\n");

                    /* 调用数据解析函数 */
                    Data_Anylize();

                    /* 重置状态机与计数 */
                    RxState = 0;
                    myusart.rxcount = 0;
                }
            }
            else
            {
                /* 正常累积接收字符 */
                if (myusart.rxcount < (sizeof(myusart.rxbuff) - 1))
                {
                    myusart.rxbuff[myusart.rxcount++] = rx;
                }
            }
            break;
        }
        default:
            break;
    }
}

/**
 * @brief UART_Debug (UART0) 串口初始化函数
 */
void UART_Init(void)
{
    /* 清除并使能 UART0 中断 */
    NVIC_ClearPendingIRQ(UART_Debug_INST_INT_IRQN);
    NVIC_EnableIRQ(UART_Debug_INST_INT_IRQN);

    myusart.rxcount = 0;
    myusart.rxover  = 0;
    RxState         = 0;
}

/**
 * @brief 串口发送单个字符（采用 DriverLib 标准阻塞接口，彻底防止时序未完成导致的乱码）
 */
void UART_Send_Byte(char ch)
{
    DL_UART_Main_transmitDataBlocking(UART_Debug_INST, (uint8_t)ch);
}

/**
 * @brief 串口发送字符串
 */
void UART_Send_Str(char *str)
{
    while (*str != '\0')
    {
        UART_Send_Byte(*str++);
    }
}

/**
 * @brief 串口发送指定长度的缓冲区（对齐截图例程）
 */
void UART_Send_Buff(uint8_t *str, uint8_t lenth)
{
    for (uint8_t i = 0; i < lenth; i++)
    {
        UART_Send_Byte((char)str[i]);
    }
}

/**
 * @brief 数据分析函数（对齐截图 Data_Anylize，并兼容直接输入数字）
 */
void Data_Anylize(void)
{
    if (myusart.rxover == 1)
    {
        myusart.rxover = 0;

        /* 1. 匹配 "Compare" 关键字，例如 "Compare:500" */
        if (strstr((char*)myusart.rxbuff, "Compare") != NULL)
        {
            if ((data_p = strstr((char*)myusart.rxbuff, ":")) != NULL)
            {
                data_p++;
                sscanf((char*)data_p, "%s", msg);
                Compare = (uint16_t)atoi((char*)msg);
            }
        }
        /* 2. 兼容直接发送纯数字，例如 "500"、"-300"、"100" */
        else if (isdigit((unsigned char)myusart.rxbuff[0]) || myusart.rxbuff[0] == '-' || myusart.rxbuff[0] == '+')
        {
            long num = atol((char*)myusart.rxbuff);
            if (num < 0) {
                Font = 1; /* 负数设为反转 */
                Compare = (uint16_t)(-num);
            } else {
                Font = 0; /* 正数设为正转 */
                Compare = (uint16_t)num;
            }
        }

        /* 3. 匹配方向关键字 */
        if (strstr((char*)myusart.rxbuff, "Forward") != NULL)
        {
            Font = 0;
        }
        if (strstr((char*)myusart.rxbuff, "Backward") != NULL)
        {
            Font = 1;
        }

        /* 4. 限幅保护 (0 ~ 1000) */
        if (Compare > MOTOR_PWM_PERIOD_MAX) {
            Compare = MOTOR_PWM_PERIOD_MAX;
        }

        /* 5. 立即同步刷新电机驱动 */
        int real_speed = (Font == 0) ? (int)Compare : -(int)Compare;
        Motor_SetSpeed(real_speed);

        /* 6. 发送 MCU 确认提示 */
        sprintf((char*)myusart.txbuff, "[MCU OK] Speed=%d, Dir=%s\r\n",
                Compare, (Font == 0) ? "Forward" : "Backward");
        UART_Send_Str((char*)myusart.txbuff);
    }
}

/**
 * @brief 串口轮询发送电机状态（左速度、右速度、方向），合成一条字符串周期发出
 * @param period_ms 发送周期，单位毫秒（建议 500ms）
 */
void UART_Poll_MotorStatus(uint32_t period_ms)
{
    static uint32_t last_send_tick = 0;
    uint32_t now = get_ticks();

    /* 距上次发送未达到周期则直接返回，非阻塞 */
    if ((now - last_send_tick) < period_ms) {
        return;
    }
    last_send_tick = now;

    int l_speed = L_MOTO_GetSpeed();
    int r_speed = R_MOTO_GetSpeed();
    int dir     = Motor_GetDirection();

    char txbuf[64];
    sprintf((char *)txbuf, "L:%d R:%d Dir:%s\r\n",
            l_speed, r_speed, (dir > 0) ? "Forward" : "Backward");
    UART_Send_Str((char *)txbuf);
}
