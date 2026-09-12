#include "USART.h"
#include "Motor.h"
#include "Encoder.h"
#include "SpeedCtrl.h"
#include "Track.h"
#include "Tick.h"
#include "Watchdog.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

/* 全局串口运行结构体 */
USART_t myusart = {
    .rxbuff       = {0},
    .rxcount      = 0,
    .rxover       = 0,
    .txbuff       = {0},
    .hex_buff     = {0},
    .hex_count    = 0,
    .hex_state    = 0,
    .packet_data  = {0},
    .packet_len   = 0,
    .packet_ready = false,
};

/* 开环控制状态变量 */
uint16_t Compare = 0;   /* 开环调速占空比 (0 ~ 1000) */
bool Font = 0;          /* 开环运动方向: 0 为正转 (+/Forward), 1 为反转 (-/Backward) */

static char* data_p;
static volatile uint32_t g_rx_total = 0;     /* 接收总字节数统计 */
static volatile uint32_t g_last_rx_tick = 0; /* 最后一个字节接收时间戳 (帧超时检测) */

/**
 * @brief UART0 (UART_Debug) 硬件中断服务函数
 * @note  具备多字节 FIFO 排空、硬件错误标志清理、双模自适应识别 (HEX包 / ASCII文本)
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
                    g_rx_total++;
                    g_last_rx_tick = get_ticks();

                    if (myusart.hex_state != 0) {
                        /* ===== 处于二进制 HEX 数据包接收状态机中 ===== */
                        if (myusart.hex_count >= sizeof(myusart.hex_buff)) {
                            /* 缓冲区溢出保护: 丢弃并复位状态机 */
                            myusart.hex_count = 0;
                            myusart.hex_state = 0;
                        } else {
                            myusart.hex_buff[myusart.hex_count++] = rx;

                            /* 1. 检测定长 5 字节数据包: [0xA5] [D1] [D2] [SUM] [0x5A] */
                            if (myusart.hex_count == 5) {
                                if (myusart.hex_buff[4] == USART_FRAME_TAIL &&
                                    (uint8_t)(myusart.hex_buff[1] + myusart.hex_buff[2]) == myusart.hex_buff[3]) {
                                    myusart.packet_data[0] = myusart.hex_buff[1];
                                    myusart.packet_data[1] = myusart.hex_buff[2];
                                    myusart.packet_len = 2;
                                    myusart.packet_ready = true;
                                    myusart.hex_state = 0;
                                    myusart.hex_count = 0;
                                }
                            }

                            /* 2. 检测变长数据包: [0xA5] [LEN] [DATA_0 ... DATA_LEN-1] [SUM] [0x5A] */
                            if (myusart.hex_state != 0) {
                                uint8_t var_len = myusart.hex_buff[1];

                                /* 2.1 LEN == 0: 总长 4 字节 [0xA5, 0x00, 0x00, 0x5A] */
                                if (myusart.hex_count == 4 && var_len == 0) {
                                    if (myusart.hex_buff[2] == 0 && myusart.hex_buff[3] == USART_FRAME_TAIL) {
                                        myusart.packet_len = 0;
                                        myusart.packet_ready = true;
                                        myusart.hex_state = 0;
                                        myusart.hex_count = 0;
                                    }
                                }
                                /* 2.2 LEN == 1: 总长 5 字节 [0xA5, 0x01, D0, D0, 0x5A] */
                                else if (myusart.hex_count == 5 && var_len == 1) {
                                    if (myusart.hex_buff[4] == USART_FRAME_TAIL &&
                                        myusart.hex_buff[2] == myusart.hex_buff[3]) {
                                        myusart.packet_data[0] = myusart.hex_buff[2];
                                        myusart.packet_len = 1;
                                        myusart.packet_ready = true;
                                        myusart.hex_state = 0;
                                        myusart.hex_count = 0;
                                    }
                                }
                                /* 2.3 LEN >= 2: 总长 var_len + 4 字节 */
                                else if (myusart.hex_count > 5) {
                                    if (var_len > USART_PACKET_MAX) {
                                        /* 载荷长度超过协议最大值，直接废弃复位 */
                                        myusart.hex_state = 0;
                                        myusart.hex_count = 0;
                                    } else {
                                        uint8_t expected_total = (uint8_t)(var_len + 4);
                                        if (myusart.hex_count == expected_total) {
                                            if (myusart.hex_buff[expected_total - 1] == USART_FRAME_TAIL) {
                                                uint8_t sum = 0;
                                                for (uint8_t i = 0; i < var_len; i++) {
                                                    sum += myusart.hex_buff[2 + i];
                                                }
                                                if (sum == myusart.hex_buff[expected_total - 2]) {
                                                    for (uint8_t i = 0; i < var_len; i++) {
                                                        myusart.packet_data[i] = myusart.hex_buff[2 + i];
                                                    }
                                                    myusart.packet_len = var_len;
                                                    myusart.packet_ready = true;
                                                }
                                            }
                                            myusart.hex_state = 0;
                                            myusart.hex_count = 0;
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        /* ===== 空闲状态: 自动识别帧头 0xA5 或 ASCII 字符 ===== */
                        if (rx == USART_FRAME_HEAD) {
                            myusart.hex_buff[0] = USART_FRAME_HEAD;
                            myusart.hex_count = 1;
                            myusart.hex_state = 1;
                        } else {
                            if (rx == '\r' || rx == '\n') {
                                if (myusart.rxcount > 0) {
                                    myusart.rxbuff[myusart.rxcount] = '\0';
                                    myusart.rxover = 1;
                                }
                            } else {
                                if (myusart.rxover == 0 &&
                                    myusart.rxcount < (sizeof(myusart.rxbuff) - 1)) {
                                    myusart.rxbuff[myusart.rxcount++] = rx;
                                }
                            }
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
 * @brief 串口硬件与中断初始化
 */
void USART_Init(void)
{
    NVIC_ClearPendingIRQ(UART_Debug_INST_INT_IRQN);
    NVIC_EnableIRQ(UART_Debug_INST_INT_IRQN);

    /* 使能错误中断，确保溢出/帧错误/噪声时能进入 ISR 及时清理恢复 */
    DL_UART_Main_enableInterrupt(UART_Debug_INST,
        DL_UART_MAIN_INTERRUPT_OVERRUN_ERROR |
        DL_UART_MAIN_INTERRUPT_FRAMING_ERROR |
        DL_UART_MAIN_INTERRUPT_NOISE_ERROR);

    memset(&myusart, 0, sizeof(myusart));
}

/**
 * @brief 发送单个字节 (阻塞式)
 */
void USART_Send_Byte(uint8_t ch)
{
    DL_UART_Main_transmitDataBlocking(UART_Debug_INST, ch);
}

/**
 * @brief 发送指定长度的数据缓冲区 (阻塞式)
 */
void USART_Send_Buff(const uint8_t *buf, uint16_t len)
{
    if (buf == NULL) return;
    for (uint16_t i = 0; i < len; i++)
    {
        USART_Send_Byte(buf[i]);
    }
}

/**
 * @brief 发送以 '\0' 结尾的字符串
 */
void USART_Send_String(const char *str)
{
    if (str == NULL) return;
    while (*str != '\0')
    {
        USART_Send_Byte((uint8_t)(*str++));
    }
}

/**
 * @brief 发送字符串 (兼容别名)
 */
void USART_Send_Str(char *str)
{
    USART_Send_String(str);
}

/**
 * @brief 发送定长 5 字节二进制 HEX 数据包: [0xA5] [D1] [D2] [SUM] [0x5A]
 */
void USART_SendPacket_Fixed(uint8_t d1, uint8_t d2)
{
    uint8_t frame[5];
    frame[0] = USART_FRAME_HEAD;
    frame[1] = d1;
    frame[2] = d2;
    frame[3] = (uint8_t)(d1 + d2);
    frame[4] = USART_FRAME_TAIL;
    USART_Send_Buff(frame, 5);
}

/**
 * @brief 发送变长二进制 HEX 数据包: [0xA5] [LEN] [DATA_0 ... DATA_LEN-1] [SUM] [0x5A]
 */
void USART_SendPacket_Var(const uint8_t *data, uint8_t len)
{
    if (len > USART_PACKET_MAX) {
        len = USART_PACKET_MAX;
    }
    uint8_t frame[USART_PACKET_MAX + 4];
    frame[0] = USART_FRAME_HEAD;
    frame[1] = len;
    uint8_t sum = 0;
    for (uint8_t i = 0; i < len; i++) {
        uint8_t val = data ? data[i] : 0;
        frame[2 + i] = val;
        sum += val;
    }
    frame[2 + len] = sum;
    frame[3 + len] = USART_FRAME_TAIL;
    USART_Send_Buff(frame, (uint16_t)(len + 4));
}

/**
 * @brief 查询是否有未读取的二进制 HEX 数据包
 */
bool USART_IsPacketReady(void)
{
    return myusart.packet_ready;
}

/**
 * @brief 获取二进制 HEX 数据包有效载荷
 */
uint8_t USART_GetPacket(uint8_t *out_data, uint8_t *out_len)
{
    if (!myusart.packet_ready) {
        if (out_len != NULL) *out_len = 0;
        return 0;
    }
    uint8_t len = myusart.packet_len;
    if (out_data != NULL && len > 0) {
        memcpy(out_data, (const void*)myusart.packet_data, len);
    }
    if (out_len != NULL) {
        *out_len = len;
    }
    myusart.packet_ready = false;
    return len;
}

/**
 * @brief 针对定长数据包 (2 字节 Payload) 的快捷读取接口
 */
bool USART_GetFixedPacket(uint8_t *d1, uint8_t *d2)
{
    if (!myusart.packet_ready || myusart.packet_len != 2) {
        return false;
    }
    if (d1 != NULL) *d1 = myusart.packet_data[0];
    if (d2 != NULL) *d2 = myusart.packet_data[1];
    myusart.packet_ready = false;
    return true;
}

/**
 * @brief 手动清除当前二进制 HEX 数据包就绪标志
 */
void USART_ClearPacket(void)
{
    myusart.packet_ready = false;
    myusart.packet_len = 0;
}

/**
 * @brief ASCII 文本指令帧处理函数 (在 main 主循环中非阻塞调用)
 */
void USART_ProcessFrame(void)
{
    uint32_t now = get_ticks();

    /* HEX 接收跨字节超时保护: 超过 50ms 未收到后续字节，强制复位 HEX 状态机 */
    if (myusart.hex_state != 0 && (now - g_last_rx_tick) > 50) {
        myusart.hex_state = 0;
        myusart.hex_count = 0;
    }

    /* ASCII 帧超时保护: 缓冲区有数据且超过 100ms 未等到 \r\n，强制封帧解析 */
    if (myusart.rxover == 0 && myusart.rxcount > 0 &&
        (now - g_last_rx_tick) > 100)
    {
        myusart.rxbuff[myusart.rxcount] = '\0';
        myusart.rxover = 1;
    }

    if (myusart.rxover != 1)
    {
        return;
    }

    /* 回显收到的文本字符串 */
    USART_Send_Buff(myusart.rxbuff, myusart.rxcount);
    USART_Send_Str("\r\n");

    /* 解析指令并执行 */
    USART_Data_Analyze();

    /* 清空接收计数，准备下一帧 */
    myusart.rxcount = 0;
}

/**
 * @brief 解析十进制浮点数 (最多支持 3 位小数)，供 PID 参数命令使用
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

/**
 * @brief ASCII 文本指令语义解析与派发函数
 */
void USART_Data_Analyze(void)
{
    if (myusart.rxover == 1)
    {
        myusart.rxover = 0;
        uint8_t valid_cmd = 0;
        uint8_t handled = 0;

        /* ---- 循迹控制指令 ---- */

        /* TSPD=<rpm>: 设置循迹基准转速 */
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
            USART_Send_Str((char*)myusart.txbuff);
        }

        /* RST?: 查询开机复位原因 */
        if (!handled && (strstr((char*)myusart.rxbuff, "RST?") != NULL ||
                         strstr((char*)myusart.rxbuff, "rst?") != NULL))
        {
            handled = 1;
            valid_cmd = 1;
            sprintf((char*)myusart.txbuff, "[MCU OK] Reset cause: 0x%02X (%s)\r\n",
                    (unsigned int)Watchdog_GetResetCause(), Watchdog_GetResetCauseStr());
            USART_Send_Str((char*)myusart.txbuff);
        }

        /* WDT_TEST: 故意死循环以测试硬件看门狗复位 */
        if (!handled && (strstr((char*)myusart.rxbuff, "WDT_TEST") != NULL ||
                         strstr((char*)myusart.rxbuff, "wdt_test") != NULL))
        {
            handled = 1;
            valid_cmd = 1;
            USART_Send_Str("[MCU WARN] Triggering WDT hang test... system will reset in 1s\r\n");
            while (1) {
                /* 挂起主循环，等待 WWDT0 硬件超时复位 */
            }
        }

        /* TLOG=<ms>: 循迹遥测流周期 (0=关闭, 建议 50~200) */
        if (!handled && ((data_p = strstr((char*)myusart.rxbuff, "TLOG")) != NULL ||
                         (data_p = strstr((char*)myusart.rxbuff, "tlog")) != NULL))
        {
            data_p += 4;
            while (*data_p == '=' || *data_p == ' ') { data_p++; }
            int val = atoi(data_p);
            USART_SetTrackLogMs((val > 0) ? (uint32_t)val : 0U);
            handled = 1;
            valid_cmd = 1;
            sprintf((char*)myusart.txbuff, "[MCU OK] TLOG=%dms (0=off)\r\n", val);
            USART_Send_Str((char*)myusart.txbuff);
        }

        /* TRK=1/0: 启用/关闭循迹 */
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
            USART_Send_Str((char*)myusart.txbuff);
        }

        /* ---- 闭环控制指令 ---- */

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
            USART_Send_Str((char*)myusart.txbuff);
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
            USART_Send_Str((char*)myusart.txbuff);
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
            USART_Send_Str((char*)myusart.txbuff);
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
            USART_Send_Str((char*)myusart.txbuff);
        }

        /* CNT: 查询编码器累计计数值并清零 */
        if ((data_p = strstr((char*)myusart.rxbuff, "CNT")) != NULL ||
            (data_p = strstr((char*)myusart.rxbuff, "cnt")) != NULL)
        {
            int32_t cnt_rl = 0, cnt_rr = 0;
            Encoder_GetAndClearCNT(&cnt_rl, &cnt_rr);
            handled = 1;
            valid_cmd = 1;
            sprintf((char*)myusart.txbuff, "[MCU OK] CNT RL=%d RR=%d (1 output rev / 20 = PPR)\r\n",
                    cnt_rl, cnt_rr);
            USART_Send_Str((char*)myusart.txbuff);
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
            USART_Send_Str((char*)myusart.txbuff);
        }

        /* ---- 开环控制指令: '+' / '-' / PWM= / Sp (闭环指令未命中时执行) ---- */
        if (!handled)
        {
            /* 1. 方向匹配 */
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

            /* 2. 匹配 "PWM=" 开环占空比 */
            if ((data_p = strstr((char*)myusart.rxbuff, "PWM")) != NULL ||
                (data_p = strstr((char*)myusart.rxbuff, "pwm")) != NULL)
            {
                data_p += 3;
                while (*data_p == '=' || *data_p == '+' || *data_p == '-') {
                    data_p++;
                }
                int val = atoi(data_p);
                if (val < 0) val = 0;
                if (val > MOTOR_PWM_PERIOD_MAX) val = MOTOR_PWM_PERIOD_MAX;
                Compare = (uint16_t)val;
                valid_cmd = 1;
            }
            /* 兼容旧版 "Sp" / "sp" 开环调速 */
            else if ((data_p = strstr((char*)myusart.rxbuff, "Sp")) != NULL ||
                     (data_p = strstr((char*)myusart.rxbuff, "sp")) != NULL)
            {
                data_p += 2;
                while (*data_p == '=' || *data_p == ' ' || *data_p == '+' || *data_p == '-') {
                    data_p++;
                }
                int val = atoi(data_p);
                if (val < 0) val = 0;
                if (val > MOTOR_PWM_PERIOD_MAX) val = MOTOR_PWM_PERIOD_MAX;
                Compare = (uint16_t)val;
                valid_cmd = 1;
            }

            /* 3. 若为有效指令，更新电机输出并回复状态 */
            if (valid_cmd)
            {
                int real_speed = (Font == 0) ? (int)Compare : -(int)Compare;
                Motor_SetSpeed(real_speed);

                sprintf((char*)myusart.txbuff, "[MCU OK] PWM=%d, Dir=%s\r\n",
                        Compare, (Font == 0) ? "+" : "-");
                USART_Send_Str((char*)myusart.txbuff);
            }
        }
    }
}

/**
 * @brief 周期性轮询并通过串口上报电机状态 (PWM/RPM/循迹状态)
 *        上报格式: "PWM L:%d R:%d | RPM L:%d R:%d | TRK:%d\r\n"
 * @param period_ms 上报周期 (单位: ms，建议 200ms)
 */
/* 紧凑循迹遥测流 (串口指令 TLOG=<ms>, 0 为关闭)
 * 格式: T,<图案HEX>,<偏差mm>,<档位>,<目标L>,<目标R>,<反馈L>,<反馈R> */
static uint32_t s_track_log_ms = 0U;      /* 0 = 关闭 */
static uint32_t s_track_last_tick = 0U;

/**
 * @brief 设置紧凑循迹遥测周期 (ms), 0 = 关闭
 */
void USART_SetTrackLogMs(uint32_t ms)
{
    s_track_log_ms = ms;
    s_track_last_tick = get_ticks();
}

void USART_Poll_MotorStatus(uint32_t period_ms)
{
    static uint32_t last_send_tick = 0;
    uint32_t now = get_ticks();

    /* 紧凑循迹遥测流: 放在状态行周期判断之前, 不受 200ms 状态周期影响 */
    if (s_track_log_ms != 0U && (now - s_track_last_tick) >= s_track_log_ms) {
        s_track_last_tick = now;
        sprintf((char*)myusart.txbuff, "T,%02X,%+d,%d,%+d,%+d,%+d,%+d\r\n",
                (unsigned)Track_GetSensor(),
                (int)Track_GetPosMM(),
                (int)Track_GetState(),
                (int)SpeedCtrl_GetTargetL(), (int)SpeedCtrl_GetTargetR(),
                (int)SpeedCtrl_GetFbL(), (int)SpeedCtrl_GetFbR());
        USART_Send_Str((char*)myusart.txbuff);
    }

    if ((now - last_send_tick) < period_ms) {
        return;
    }
    last_send_tick = now;

    int l_speed = L_MOTO_GetSpeed();
    int r_speed = R_MOTO_GetSpeed();

    char txbuf[160];
    sprintf((char *)txbuf,
            "PWM L:%d R:%d | RPM L:%d R:%d | TRK:%d | G:%02X P:%+d B:%d TL:%+d TR:%+d\r\n",
            l_speed, r_speed,
            Encoder_GetLRPM(), Encoder_GetRRPM(),
            (int)Track_GetState(),
            (unsigned)Track_GetSensor(), (int)Track_GetPosMM(), (int)Track_GetState(),
            (int)SpeedCtrl_GetTargetL(), (int)SpeedCtrl_GetTargetR());
    USART_Send_Str((char *)txbuf);
}

/**
 * @brief 获取自上电以来 UART 实际接收到的总字节数 (RX 链路诊断)
 */
uint32_t USART_GetRxTotal(void)
{
    return g_rx_total;
}
