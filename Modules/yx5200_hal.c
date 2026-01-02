#include "yx5200_hal.h"
#include "oled.h"      // 解决 OLED_PrintASCIIString, afont16x8, OLED_COLOR_NORMAL 未定义错误
#include <stdio.h>     // 解决 sprintf 隐式声明警告
// 发送指令的基础函数
// 格式: 7E FF 06 CMD FDBK DATA_H DATA_L EF
void YX5200_SendCommand(uint8_t command, uint16_t data)
{
    uint8_t send_buf[8];
    
    send_buf[0] = 0x7E; // 起始位
    send_buf[1] = 0xFF; // 版本
    send_buf[2] = 0x06; // 数据长度
    send_buf[3] = command; // 命令
    send_buf[4] = 0x00; // 不需要反馈 (01为需要反馈)
    send_buf[5] = (uint8_t)(data >> 8); // 数据高位
    send_buf[6] = (uint8_t)(data & 0xFF); // 数据低位
    send_buf[7] = 0xEF; // 结束位
    
    // 使用 HAL 库发送
    HAL_UART_Transmit(&huart2, send_buf, 8, 100);
}

// 初始化（主要是复位一下）
void YX5200_Init(void)
{
    // 这里不需要像标准库那样配GPIO，CubeMX已经配好了
    // 我们可以发送一个复位指令，或者选择TF卡设备
    // 0x09 0x02 代表选择TF卡
    YX5200_SendCommand(CMD_SEL_DEV, 0x02);
    HAL_Delay(200); // 模块启动需要一点时间
}

// 播放指定序号的歌曲 (1-65535)
void YX5200_PlayTrack(uint16_t index)
{
    YX5200_SendCommand(CMD_PLAY_INDEX, index);
}

// 设置音量 (0-30)
void YX5200_SetVolume(uint8_t volume)
{
    if(volume > 30) volume = 30;
    YX5200_SendCommand(CMD_SET_VOLUME, volume);
}

uint8_t YX5200_IsConnected(void)
{
    // 查询状态指令: 7E FF 06 42 00 00 00 EF
    uint8_t send_buf[8] = {0x7E, 0xFF, 0x06, 0x42, 0x00, 0x00, 0x00, 0xEF};
    uint8_t rx_buf[10] = {0}; // 准备接收10个字节
    HAL_StatusTypeDef status;
    char debug_buf[32]; // 用于存放要打印到屏幕的字符串

    // 1. 发送查询指令
    HAL_UART_Transmit(&huart2, send_buf, 8, 100);

    // 2. 尝试接收数据 (超时设为 500ms)
    // HAL_UART_Receive 会阻塞，直到收满 10 个字节 或 超时
    status = HAL_UART_Receive(&huart2, rx_buf, 10, 500);

    // 3. 【核心修改】把收到的原始数据打印到屏幕上，让我们看看它到底是啥
    // 我们打印前3个字节，通常如果是正常回复，应该是 7E FF 06 ...
    OLED_NewFrame();
    sprintf(debug_buf, "Raw:%02X %02X %02X", rx_buf[0], rx_buf[1], rx_buf[2]);
    OLED_PrintASCIIString(0, 0, debug_buf, &afont16x8, OLED_COLOR_NORMAL);
    
    // 4. 【宽松判断逻辑】
    // 只要 status 是 HAL_OK，说明 STM32 实实在在收到了 10 个字节的数据
    // 不管内容是不是 0x7E 开头，我们都算它成功
    if (status == HAL_OK)
    {
        OLED_PrintASCIIString(0, 20, "Data Received!", &afont16x8, OLED_COLOR_NORMAL);
        OLED_ShowFrame();
        HAL_Delay(1000); // 暂停一下让你看清屏幕
        return 1; // 返回 1，告诉主函数“连接成功”
    }
    
    // 如果是 TIMEOUT，说明连乱码都没收到，那是真没连上
    OLED_PrintASCIIString(0, 20, "Rx Timeout...", &afont16x8, OLED_COLOR_NORMAL);
    OLED_ShowFrame();
    HAL_Delay(1000);
    return 0;
}