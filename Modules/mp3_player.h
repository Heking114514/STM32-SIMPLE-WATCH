/*
 * mp3_player.h
 * YX5200 非阻塞驱动 (带内部状态管理)
 */
#ifndef __MP3_PLAYER_H
#define __MP3_PLAYER_H

#include "main.h"
#include "stdint.h"
#include <stdbool.h>
// MP3 初始化状态机定义
typedef enum {
    MP3_STATE_IDLE,         // 空闲/未初始化
    MP3_STATE_INITIALIZING, // 正在发送复位指令
    MP3_STATE_WAIT_BOOT,    // 等待模块复位启动
    MP3_STATE_QUERY_TOTAL,  // 发送查询总数指令
    MP3_STATE_WAIT_TOTAL,   // 等待串口中断接收总数
    MP3_STATE_READY,        // 初始化完成，工作正常
    MP3_STATE_ERROR         // 初始化超时或失败
} MP3_State_t;

// --- 全局变量 ---
extern uint16_t MP3_TotalTracks; // 总曲目数
extern MP3_State_t MP3_State;    // 当前模块初始化状态

// --- 核心函数 ---
void MP3_Loop_Handler(void);

void MP3_Init_Async(void);       // 启动异步初始化
void MP3_Loop_Task(void);        // 核心任务循环 (放在 while(1) 中)

// --- 控制接口 ---
void MP3_PlayTrack(uint16_t index); // 播放指定序号
void MP3_Play(void);                // 播放/恢复
void MP3_Pause(void);               // 暂停
void MP3_TogglePlayback(void);      // 【新增】在播放和暂停之间切换
void MP3_Next(void);                // 下一曲
void MP3_Prev(void);                // 上一曲
void MP3_SetVolume(uint8_t vol);    // 设置音量 (0-30)

void MP3_SetMuteStatus(bool mute);
bool MP3_GetMuteStatus(void);

// --- 状态查询 ---
uint8_t MP3_IsPlaying(void);        // 【新增】返回 1 表示正在播放，0 表示暂停
// 在 mp3_player.h 中添加声明：
uint16_t MP3_GetCurrentTrack(void);
/* mp3_player.h */
uint16_t MP3_QueryCurrentTrack(void);

#endif