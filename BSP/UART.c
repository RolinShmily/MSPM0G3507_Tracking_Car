#include "UART.h"
#include "Motor.h"
#include "Encoder.h"
#include "SpeedCtrl.h"
#include "Track.h"
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
bool Font = 0;          /* 初始方向为 0 (正向/Forward) */

static char* data_p;
static uint8_t RxState = 0;
static volatile uint32_t g_rx_total = 0;   /* RX 收到的总字节数 (诊断用) */
static volatile uint32_t g_last_rx_tick = 0; /* 最近一次收到字节的时间戳 (帧超时用) */

/**
 * @brief UART0 (UART_Debug) 中断服务函数
 * @note  排查并清除所有挂起的中断标志，发生溢出/帧错误/噪声时排空 FIFO 并清除标志，防止外设挂死
 */
void UART_Debug_INST_IRQHandler(void)
{
    DL_UART_IIDX iidx;
    while ((iidx = DL_UART_Main_getPendingInterrupt(UART_Debug_INST)) != DL_UART_MAIN_IIDX_NO_INTERRUPT) {
        switch (iidx) {
            case DL_UART_MAIN_IIDX_RX:
            case DL_UART_MAIN_IIDX_RX_TIMEOUT_ERROR: {
                while (!DL_UART_Main_isRXFIFOEmpty(UART_Debug_INST)) {
                    uint8_t rx = DL_UART_Main_receiveData(UART_Debug_INST);
                    g_rx_total++;            /* 统计实际收到的字节数 (诊断 RX 链路) */
                    g_last_rx_tick = get_ticks(); /* 刷新最新字节时间戳 (帧超时用) */

                    /* 收到 \r 或 \n 判定为一帧接收完成 */
                    if (rx == '\r' || rx == '\n')
                    {
                        if (myusart.rxcount > 0)
                        {
                            /* 一帧接收完成: 添加字符串结束符, 置帧就绪标志 */
                            myusart.rxbuff[myusart.rxcount] = '\0';
                            myusart.rxover = 1;
                        }
                    }
                    else
                    {
                        /* 正常累积接收字符 (帧未处理前继续缓存, 由主循环处理后清零) */
                        if (myusart.rxover == 0 &&
                            myusart.rxcount < (sizeof(myusart.rxbuff) - 1))
                        {
                            myusart.rxbuff[myusart.rxcount++] = rx;
                        }
                    }
                }
                break;
            }
            case DL_UART_MAIN_IIDX_OVERRUN_ERROR:
            case DL_UART_MAIN_IIDX_BREAK_ERROR:
            case DL_UART_MAIN_IIDX_PARITY_ERROR:
            case DL_UART_MAIN_IIDX_FRAMING_ERROR:
            case DL_UART_MAIN_IIDX_NOISE_ERROR: {
                /* 发生溢出、帧错误、校验错误或噪声错误时，排空 RX FIFO 并清除错误中断标志，防止外设挂死 */
                uint8_t dummy[16];
                DL_UART_Main_drainRXFIFO(UART_Debug_INST, dummy, sizeof(dummy));
                DL_UART_Main_clearInterruptStatus(UART_Debug_INST,
                    DL_UART_MAIN_INTERRUPT_OVERRUN_ERROR |
                    DL_UART_MAIN_INTERRUPT_BREAK_ERROR   |
                    DL_UART_MAIN_INTERRUPT_PARITY_ERROR  |
                    DL_UART_MAIN_INTERRUPT_FRAMING_ERROR |
                    DL_UART_MAIN_INTERRUPT_NOISE_ERROR);
                break;
            }
            default:
                break;
        }
    }
}

/**
 * @brief 帧处理函数: 检测到一帧接收完成后, 在主循环上下文中完成回显、解析与应答
 * @note  避免在中断内执行 sprintf 与阻塞发送 (非可重入 + 9600 波特率下耗时约 45ms),
 *        同时消除与主循环 UART_Poll_MotorStatus 中 sprintf 的重入冲突
 */
void UART_ProcessFrame(void)
{
    /* 帧超时兜底: 缓冲区有数据但超过 100ms 未等到帧尾, 强制完成该帧
     * (应对帧尾字符丢失/噪声预污染缓冲区等异常情况) */
    if (myusart.rxover == 0 && myusart.rxcount > 0 &&
        (get_ticks() - g_last_rx_tick) > 100)
    {
        myusart.rxbuff[myusart.rxcount] = '\0';
        myusart.rxover = 1;
    }

    if (myusart.rxover != 1)
    {
        return;
    }

    /* 回显所收到的缓冲区内容 */
    UART_Send_Buff(myusart.rxbuff, myusart.rxcount);
    UART_Send_Str("\r\n");

    /* 调用数据解析函数 (内部清除 rxover) */
    Data_Anylize();

    /* 重置状态机与计数, 准备接收下一帧 */
    RxState = 0;
    myusart.rxcount = 0;
}

/**
 * @brief UART_Debug (UART0) 串口初始化函数
 */
void UART_Init(void)
{
    NVIC_ClearPendingIRQ(UART_Debug_INST_INT_IRQN);
    NVIC_EnableIRQ(UART_Debug_INST_INT_IRQN);

    /* 使能错误中断，确保发生溢出/帧错误/噪声时能进入 ISR 及时清除与恢复 */
    DL_UART_Main_enableInterrupt(UART_Debug_INST,
        DL_UART_MAIN_INTERRUPT_OVERRUN_ERROR |
        DL_UART_MAIN_INTERRUPT_FRAMING_ERROR |
        DL_UART_MAIN_INTERRUPT_NOISE_ERROR);

    myusart.rxcount = 0;
    myusart.rxover  = 0;
    RxState         = 0;
}

/**
 * @brief 串口发送单个字符（标准阻塞发送，彻底防止时钟不同步乱码）
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
 * @brief 串口发送指定长度的缓冲区
 */
void UART_Send_Buff(uint8_t *str, uint8_t lenth)
{
    for (uint8_t i = 0; i < lenth; i++)
    {
        UART_Send_Byte((char)str[i]);
    }
}

/**
 * @brief 简化指令数据分析函数
 *        - 匹配 '+' : 正向
 *        - 匹配 '-' : 反向
 *        - 匹配 'Sp' / 'sp' : 提取后方数值作为 Compare (0~1000)
 */
/**
 * @brief 解析十进制浮点数 (可选小数部分, 最多3位), 供 PID 参数命令使用
 * @note  不依赖 atof, 避免 microlib 浮点格式化开销
 */
static float Parse_Float(const char *s)
{
    float v = 0.0f;
    int neg = 0;
    float frac = 0.1f;

    if (*s == '-') { neg = 1; s++; }
    else if (*s == '+') { s++; }

    while (*s >= '0' && *s <= '9') {
        v = v * 10.0f + (float)(*s++ - '0');
    }
    if (*s == '.') {
        s++;
        while (*s >= '0' && *s <= '9') {
            v += (float)(*s++ - '0') * frac;
            frac *= 0.1f;
        }
    }
    return neg ? -v : v;
}

void Data_Anylize(void)
{
    if (myusart.rxover == 1)
    {
        myusart.rxover = 0;
        uint8_t valid_cmd = 0;
        uint8_t handled = 0;   /* 新命令已处理时, 跳过旧版 Sp/+/- 逻辑 */

        /* ---- 循迹控制命令 ---- */

        /* TSPD=<rpm>: 设置循迹基础转速 */
        if ((data_p = strstr((char*)myusart.rxbuff, "TSPD")) != NULL ||
            (data_p = strstr((char*)myusart.rxbuff, "tspd")) != NULL)
        {
            data_p += 4;
            while (*data_p == '=' || *data_p == ' ') { data_p++; }
            int val = atoi(data_p);
            Track_SetBaseSpeed(val);
            handled = 1;
            valid_cmd = 1;
            sprintf((char*)myusart.txbuff, "[MCU OK] Track base speed=%d RPM\r\n", (int)Track_GetBaseSpeed());
            UART_Send_Str((char*)myusart.txbuff);
        }

        /* TRK=1/0: 使能/关闭循迹 */
        if (!handled && ((data_p = strstr((char*)myusart.rxbuff, "TRK")) != NULL ||
                         (data_p = strstr((char*)myusart.rxbuff, "trk")) != NULL))
        {
            data_p += 3;
            while (*data_p == '=' || *data_p == ' ') { data_p++; }
            int val = atoi(data_p);
            if (val) {
                SpeedCtrl_Enable(1);
                Track_Enable(1);
            } else {
                Track_Enable(0);
                SpeedCtrl_Enable(0);
            }
            handled = 1;
            valid_cmd = 1;
            sprintf((char*)myusart.txbuff, "[MCU OK] Track enable=%d\r\n",
                    Track_IsEnabled());
            UART_Send_Str((char*)myusart.txbuff);
        }

        /* ---- 闭环控制命令 (参考报告表 5-1) ---- */

        /* SPD=<rpm>: 设置闭环目标转速 (左右轮相同) */
        if (!handled && ((data_p = strstr((char*)myusart.rxbuff, "SPD")) != NULL ||
                         (data_p = strstr((char*)myusart.rxbuff, "spd")) != NULL))
        {
            data_p += 3;
            while (*data_p == '=' || *data_p == ' ') { data_p++; }
            int val = atoi(data_p);
            SpeedCtrl_SetTarget(val);
            handled = 1;
            valid_cmd = 1;
            sprintf((char*)myusart.txbuff, "[MCU OK] PID target=%d RPM\r\n", val);
            UART_Send_Str((char*)myusart.txbuff);
        }

        /* KP=<f>: 设置比例系数 */
        if ((data_p = strstr((char*)myusart.rxbuff, "KP")) != NULL ||
            (data_p = strstr((char*)myusart.rxbuff, "kp")) != NULL)
        {
            data_p += 2;
            while (*data_p == '=' || *data_p == ' ') { data_p++; }
            SpeedCtrl_SetParams(Parse_Float(data_p),
                                SpeedCtrl_GetKi(), SpeedCtrl_GetKd());
            handled = 1;
            valid_cmd = 1;
            sprintf((char*)myusart.txbuff, "[MCU OK] KP=%d (x0.001)\r\n",
                    (int)(SpeedCtrl_GetKp() * 1000.0f));
            UART_Send_Str((char*)myusart.txbuff);
        }

        /* KI=<f>: 设置积分系数 */
        if ((data_p = strstr((char*)myusart.rxbuff, "KI")) != NULL ||
            (data_p = strstr((char*)myusart.rxbuff, "ki")) != NULL)
        {
            data_p += 2;
            while (*data_p == '=' || *data_p == ' ') { data_p++; }
            SpeedCtrl_SetParams(SpeedCtrl_GetKp(), Parse_Float(data_p),
                                SpeedCtrl_GetKd());
            handled = 1;
            valid_cmd = 1;
            sprintf((char*)myusart.txbuff, "[MCU OK] KI=%d (x0.001)\r\n",
                    (int)(SpeedCtrl_GetKi() * 1000.0f));
            UART_Send_Str((char*)myusart.txbuff);
        }

        /* KD=<f>: 设置微分系数 */
        if ((data_p = strstr((char*)myusart.rxbuff, "KD")) != NULL ||
            (data_p = strstr((char*)myusart.rxbuff, "kd")) != NULL)
        {
            data_p += 2;
            while (*data_p == '=' || *data_p == ' ') { data_p++; }
            SpeedCtrl_SetParams(SpeedCtrl_GetKp(), SpeedCtrl_GetKi(),
                                Parse_Float(data_p));
            handled = 1;
            valid_cmd = 1;
            sprintf((char*)myusart.txbuff, "[MCU OK] KD=%d (x0.001)\r\n",
                    (int)(SpeedCtrl_GetKd() * 1000.0f));
            UART_Send_Str((char*)myusart.txbuff);
        }

        /* CNT: 查询并清零编码器原始脉冲计数 (PPR 实测用:
         * 手转输出轴一整圈, 再次查询, 读数绝对值/20 = PPR) */
        if ((data_p = strstr((char*)myusart.rxbuff, "CNT")) != NULL ||
            (data_p = strstr((char*)myusart.rxbuff, "cnt")) != NULL)
        {
            int32_t cnt_rl = 0, cnt_rr = 0;
            Encoder_GetAndClearCNT(&cnt_rl, &cnt_rr);
            handled = 1;
            valid_cmd = 1;
            sprintf((char*)myusart.txbuff, "[MCU OK] CNT RL=%d RR=%d (1 output rev / 20 = PPR)\r\n",
                    cnt_rl, cnt_rr);
            UART_Send_Str((char*)myusart.txbuff);
        }

        /* PID=1/0: 使能/关闭速度闭环 */
        if ((data_p = strstr((char*)myusart.rxbuff, "PID")) != NULL ||
            (data_p = strstr((char*)myusart.rxbuff, "pid")) != NULL)
        {
            data_p += 3;
            while (*data_p == '=' || *data_p == ' ') { data_p++; }
            int val = atoi(data_p);
            SpeedCtrl_Enable(val ? 1 : 0);
            handled = 1;
            valid_cmd = 1;
            sprintf((char*)myusart.txbuff, "[MCU OK] PID enable=%d\r\n",
                    SpeedCtrl_IsEnabled());
            UART_Send_Str((char*)myusart.txbuff);
        }

        /* ---- 旧版开环命令: '+' / '-' / Sp<value> (闭环命令未处理时才执行) ---- */
        if (!handled)
        {
            /* 1. 匹配方向命令 '+' 或 '-' */
            if (strchr((char*)myusart.rxbuff, '+') != NULL)
            {
                Font = 0; /* 正向 */
                valid_cmd = 1;
            }
            if (strchr((char*)myusart.rxbuff, '-') != NULL)
            {
                Font = 1; /* 反向 */
                valid_cmd = 1;
            }

            /* 2. 匹配 "PWM=" 速度命令 (例如 "PWM=100", "pwm=500") */
            if ((data_p = strstr((char*)myusart.rxbuff, "PWM")) != NULL ||
                (data_p = strstr((char*)myusart.rxbuff, "pwm")) != NULL)
            {
                data_p += 3;   /* 跳过关键字 "PWM" (3字符), 再跳过 '=' 等符号 */
                while (*data_p == '=' || *data_p == '+' || *data_p == '-') {
                    data_p++;
                }
                int val = atoi(data_p);
                if (val < 0) val = 0;
                if (val > MOTOR_PWM_PERIOD_MAX) val = MOTOR_PWM_PERIOD_MAX;
                Compare = (uint16_t)val;
                valid_cmd = 1;
            }

            /* 3. 若为有效指令，立即更新电机输出并回复状态 */
            if (valid_cmd)
            {
                int real_speed = (Font == 0) ? (int)Compare : -(int)Compare;
                Motor_SetSpeed(real_speed);

                sprintf((char*)myusart.txbuff, "[MCU OK] PWM=%d, Dir=%s\r\n",
                        Compare, (Font == 0) ? "+" : "-");
                UART_Send_Str((char*)myusart.txbuff);
            }
        }
    }
}

/**
 * @brief 串口轮询发送电机状态: PWM 给定值与编码器实测转速 (RPM) 及循迹状态
 *        精简上报格式: "PWM L:%d R:%d | RPM L:%d R:%d | TRK:%d\r\n"
 *        报文长度仅 ~35 字节, 9600 波特率下耗时 < 35ms, 消除对主循环的长时间阻塞
 * @param period_ms 发送周期，单位毫秒（建议 200ms）
 */
void UART_Poll_MotorStatus(uint32_t period_ms)
{
    static uint32_t last_send_tick = 0;
    uint32_t now = get_ticks();

    if ((now - last_send_tick) < period_ms) {
        return;
    }
    last_send_tick = now;

    int l_speed = L_MOTO_GetSpeed();
    int r_speed = R_MOTO_GetSpeed();

    char txbuf[64];
    sprintf((char *)txbuf, "PWM L:%d R:%d | RPM L:%d R:%d | TRK:%d\r\n",
            l_speed, r_speed,
            Encoder_GetLRPM(), Encoder_GetRRPM(),
            (int)Track_GetState());
    UART_Send_Str((char *)txbuf);
}

/**
 * @brief 获取 UART 自上电以来收到的总字节数 (诊断 RX 链路用)
 */
uint32_t UART_GetRxTotal(void)
{
    return g_rx_total;
}
