#ifndef __YX5200_HAL_H
#define __YX5200_HAL_H

#include "stm32f1xx_hal.h" // 根据你的芯片型号修改，F103是f1xx
#include "stdint.h"
// 依赖的串口句柄，在main.c中定义
extern UART_HandleTypeDef huart2;

// YX5200 指令定义
#define CMD_NEXT_SONG     0x01  // 下一曲
#define CMD_PREV_SONG     0x02  // 上一曲
#define CMD_PLAY_INDEX    0x03  // 指定曲目索引播放 (0x03 DataH DataL)
#define CMD_VOLUME_UP     0x04  // 音量+
#define CMD_VOLUME_DOWN   0x05  // 音量-
#define CMD_SET_VOLUME    0x06  // 设置音量 (0-30)
#define CMD_SET_EQ        0x07  // 设置EQ
#define CMD_PLAY_MODE     0x08  // 设置循环模式
#define CMD_SEL_DEV       0x09  // 选择存储设备
#define CMD_SLEEP         0x0A  // 进入休眠
#define CMD_RESET         0x0C  // 复位
#define CMD_PLAY          0x0D  // 播放
#define CMD_PAUSE         0x0E  // 暂停
#define CMD_QUERY_STATUS  0x42  // 查询当前状态
#define CMD_QUERY_VOLUME  0x43  // 查询当前音量
#define CMD_QUERY_TOTAL   0x48  // 查询TF卡总文件数

void YX5200_Init(void);
void YX5200_SendCommand(uint8_t command, uint16_t data);
void YX5200_PlayTrack(uint16_t index);
void YX5200_SetVolume(uint8_t volume);

uint8_t YX5200_IsConnected(void);

#endif