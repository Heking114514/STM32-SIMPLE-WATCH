/*
 * mp3_player.c
 * 回退到“阻塞式初始化”版本 - 也就是你测试成功的那个逻辑
 * 不需要开启中断，不需要复杂的 State Machine
 */
#include "mp3_player.h"
#include "usart.h"
#include <stdio.h>
#include "key.h"

extern UART_HandleTypeDef huart2;

// --- 全局变量 ---
uint16_t MP3_TotalTracks = 0;
MP3_State_t MP3_State = MP3_STATE_READY; // 默认给个 READY，防止报错
// 状态：1=播放中, 0=暂停。初始化后默认认为它在播放
static uint8_t mp3_is_playing = 1; 
static bool g_is_muted = false;



static void MP3_Amp_On(void)
{
    HAL_GPIO_WritePin(APK_OFF_GPIO_Port, APK_OFF_Pin, GPIO_PIN_RESET);
}

/**
 * @brief 关闭功放 (APK_OFF 引脚拉高)
 */
static void MP3_Amp_Off(void)
{
    HAL_GPIO_WritePin(APK_OFF_GPIO_Port, APK_OFF_Pin, GPIO_PIN_SET);
}



// ==========================================
//              底层指令发送
// ==========================================

void MP3_SetMuteStatus(bool mute)
{
    g_is_muted = mute;
    
    if (g_is_muted) {
        // 如果设置为静音，立刻物理关闭功放
        MP3_Amp_Off();
    } else {
        // 如果解除静音，且当前正在播放，则恢复功放
        if (mp3_is_playing) {
            MP3_Amp_On();
        }
    }
}

bool MP3_GetMuteStatus(void)
{
    return g_is_muted;
}


static void MP3_SendCmd(uint8_t cmd, uint16_t data)
{
    uint8_t send_buf[10];
    uint16_t checksum;
    
    send_buf[0] = 0x7E; // 起始
    send_buf[1] = 0xFF; // 版本
    send_buf[2] = 0x06; // 长度
    send_buf[3] = cmd;  // 指令
    send_buf[4] = 0x00; // 0=不反馈, 1=反馈
    send_buf[5] = (uint8_t)(data >> 8);   // 数据高位
    send_buf[6] = (uint8_t)(data & 0xFF); // 数据低位
    
    // 计算校验和 (0 - (FF+06+CMD+00+DATA_H+DATA_L))
    checksum = 0xFFFF - (send_buf[1] + send_buf[2] + send_buf[3] + send_buf[4] + send_buf[5] + send_buf[6]) + 1;
    
    send_buf[7] = (uint8_t)(checksum >> 8);   // 校验高位
    send_buf[8] = (uint8_t)(checksum & 0xFF); // 校验低位
    
    send_buf[9] = 0xEF; // 结束位
    
    // 发送 10 个字节
    HAL_UART_Transmit(&huart2, send_buf, 10, 100);
}

// ==========================================
//              初始化 (阻塞式)
// ==========================================
// 这里删除了 Async 和 Loop_Task，改回最原始可靠的 Init
void MP3_Init_Async(void)
{
    uint8_t rx_buf[30] = {0}; 
    uint8_t dummy_byte;

    // 步骤1: 发送复位指令，让模块进入一个已知的初始状态。
    MP3_SendCmd(0x0C, 0); 
    
    // 步骤2: 给予足够长的延时，等待模块完成重启。
    HAL_Delay(800); 

    // 步骤3: 【关键】彻底清空串口接收缓存区，丢弃模块重启时可能发出的所有杂乱数据。
    while (HAL_UART_Receive(&huart2, &dummy_byte, 1, 10) == HAL_OK);

    // 步骤4: 发送查询总文件数的指令。
    uint8_t query_cmd[] = {0x7E, 0xFF, 0x06, 0x48, 0x00, 0x00, 0x00, 0xEF};
    HAL_UART_Transmit(&huart2, query_cmd, sizeof(query_cmd), 100);

    // 步骤5: 尝试接收数据。我们不检查返回值，因为我们知道它很可能会超时，
    // 但数据依然会被接收到缓冲区中。
    HAL_UART_Receive(&huart2, rx_buf, sizeof(rx_buf), 500);

    // 步骤6: 【关键】对接收到的数据进行扫描解析，查找有效的数据帧。
    MP3_TotalTracks = 0; // 先清零
    for(int i = 0; i <= (sizeof(rx_buf) - 10); i++)
    {
        // 特征：帧头0x7E, 指令码0x48在第4字节(i+3), 帧尾0xEF在第10字节(i+9)
        if(rx_buf[i]   == 0x7E &&
           rx_buf[i+3] == 0x48 &&
           rx_buf[i+9] == 0xEF)
        {
            // 找到了！提取数据
            MP3_TotalTracks = (rx_buf[i+5] << 8) | rx_buf[i+6];
            break; // 成功找到，退出循环
        }
    }
    
    // 步骤7: (可选) 兜底：如果查询失败，给一个默认值。
    // 如果上面循环没找到，MP3_TotalTracks 将保持为0。
    // 在调试时，你可以根据这个值是否为0来判断函数是否执行成功。
    // if(MP3_TotalTracks == 0) MP3_TotalTracks = 10;
}

// 既然是阻塞初始化，不需要 Loop 任务了，留个空函数防止报错
void MP3_Loop_Task(void)
{
    // 空函数
}

// ==========================================
//              控制接口
// ==========================================
// 这些函数发送指令极快，不会阻塞主循环

void MP3_PlayTrack(uint16_t index)
{
    MP3_Amp_On(); // 先开功放
    HAL_Delay(10);  // 等待功放稳定
    MP3_SendCmd(0x03, index);
    mp3_is_playing = 1;
}

void MP3_Play(void)
{
    MP3_Amp_On();
    HAL_Delay(10);
    MP3_SendCmd(0x0D, 0);
    mp3_is_playing = 1;
}

void MP3_Pause(void)
{
    MP3_SendCmd(0x0E, 0);
    mp3_is_playing = 0;
    MP3_Amp_Off(); // 暂停时关闭功放
}

/**
 * @brief 【新增】停止播放函数
 */
void MP3_Stop(void)
{
    MP3_SendCmd(0x16, 0); // 发送停止指令
    mp3_is_playing = 0;
    MP3_Amp_Off(); // 停止时关闭功放
}

void MP3_TogglePlayback(void)
{
    if (mp3_is_playing) {
        MP3_Pause();
    } else {
        MP3_Play();
    }
}

void MP3_Next(void)
{
    MP3_Amp_On();
    HAL_Delay(10);
    MP3_SendCmd(0x01, 0);
    mp3_is_playing = 1;
}

void MP3_Prev(void)
{
    MP3_Amp_On();
    HAL_Delay(10);
    MP3_SendCmd(0x02, 0);
    mp3_is_playing = 1;
}

void MP3_SetVolume(uint8_t vol)
{
    if(vol > 30) vol = 30;
    MP3_SendCmd(0x06, vol);
}

uint8_t MP3_IsPlaying(void)
{
    return mp3_is_playing;
}



void MP3_Loop_Handler(void)
{
    // 如果当前状态不是“播放中”，就没必要监听了，直接退出
    if (!mp3_is_playing) {
        return;
    }

    uint8_t rx_buf[10];

    // 尝试以非阻塞的方式接收10个字节的数据
    if (HAL_UART_Receive(&huart2, rx_buf, 10, 10) == HAL_OK) // 超时时间很短
    {
        // 检查是不是一个有效的返回帧
        if (rx_buf[0] == 0x7E && rx_buf[9] == 0xEF)
        {
            // 根据手册，TF卡播放完成的上报指令是 0x3D
            if (rx_buf[3] == 0x3D)
            {
                // 歌曲播放完了！
                mp3_is_playing = 0; // 更新状态
                MP3_Amp_Off();      // 关闭功放
            }
        }
    }
}

uint16_t MP3_QueryCurrentTrack(void)
{
    uint8_t query_cmd[] = {0x7E, 0xFF, 0x06, 0x4C, 0x00, 0x00, 0x00, 0xEF}; // 0x4C 是查询当前TF卡曲目指令
    uint8_t rx_buf[10] = {0};
    uint16_t current_track = 0; // 默认返回0表示失败

    // 1. 发送查询指令前，先简单清空一下缓存，防止读到旧数据
    uint8_t dummy;
    while(HAL_UART_Receive(&huart2, &dummy, 1, 2) == HAL_OK);

    // 2. 发送查询指令
    HAL_UART_Transmit(&huart2, query_cmd, sizeof(query_cmd), 10);

    // 3. 等待接收返回的数据，超时时间设为100ms足够了
    // 这里我们依然不判断返回值，直接去解析数据
    HAL_UART_Receive(&huart2, rx_buf, sizeof(rx_buf), 100);

    // 4. 解析数据
    // 检查返回帧的有效性，指令码是 0x4C
    if (rx_buf[0] == 0x7E && rx_buf[9] == 0xEF && rx_buf[3] == 0x4C)
    {
        // 数据在第5和第6字节
        current_track = (rx_buf[5] << 8) | rx_buf[6];
    }
    
    return current_track;
}