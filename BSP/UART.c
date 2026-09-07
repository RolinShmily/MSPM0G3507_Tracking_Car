#include "UART.h"
#include "Motor.h"
#include "Encoder.h"
#include "SpeedCtrl.h"
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
 * @note  逐字节接收字符；收到回车/换行表示一帧结束，
 *        仅添加 '\0' 结束符并置 rxover=1 标志，
 *        回显与解析应答由主循环调用 UART_ProcessFrame() 完成（中断内不做耗时操作）
 */
void UART_Debug_INST_IRQHandler(void)
{
    switch (DL_UART_Main_getPendingInterrupt(UART_Debug_INST)) {
        case DL_UART_MAIN_IIDX_RX: {
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
            break;
        }
        default:
            break;
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

        /* ---- 闭环控制命令 (参考报告表 5-1) ---- */

        /* SPD=<rpm>: 设置闭环目标转速 (左右轮相同) */
        if ((data_p = strstr((char*)myusart.rxbuff, "SPD")) != NULL ||
            (data_p = strstr((char*)myusart.rxbuff, "spd")) != NULL)
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

            /* 2. 匹配 "Sp" 或 "sp" 速度命令 (例如 "Sp100", "sp500") */
            if ((data_p = strstr((char*)myusart.rxbuff, "Sp")) != NULL ||
                (data_p = strstr((char*)myusart.rxbuff, "sp")) != NULL)
            {
                data_p += 2;
                while (*data_p == ' ' || *data_p == '+' || *data_p == '-') {
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

                sprintf((char*)myusart.txbuff, "[MCU OK] Speed=%d, Dir=%s\r\n",
                        Compare, (Font == 0) ? "+" : "-");
                UART_Send_Str((char*)myusart.txbuff);
            }
        }
    }
}

/**
 * @brief 串口轮询发送电机状态: PWM 给定值 (带方向) 与编码器实测转速 (RPM)
 *        上报格式: "PWM L:<左> R:<右> Dir:<+/-> | RPM L:<左> R:<右>"
 * @param period_ms 发送周期，单位毫秒（建议 500ms）
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
    int dir     = Motor_GetDirection();

    char txbuf[128];
    sprintf((char *)txbuf, "PWM L:%d R:%d Dir:%s | RPM L:%d R:%d | RX:%u C:%u D:%02X %02X %02X %02X\r\n",
            l_speed, r_speed, (dir > 0) ? "+" : "-",
            Encoder_GetLRPM(), Encoder_GetRRPM(), (unsigned)g_rx_total,
            (unsigned)myusart.rxcount,
            myusart.rxbuff[0], myusart.rxbuff[1], myusart.rxbuff[2], myusart.rxbuff[3]);
    UART_Send_Str((char *)txbuf);
}

/**
 * @brief 获取 UART 自上电以来收到的总字节数 (诊断 RX 链路用)
 */
uint32_t UART_GetRxTotal(void)
{
    return g_rx_total;
}
