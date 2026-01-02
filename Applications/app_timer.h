#ifndef __APP_TOOLS_H
#define __APP_TOOLS_H

#include <stdint.h>
#include <stdbool.h>

// --- 秒表 App ---
void App_Stopwatch_Loop(void);

// --- 闹钟 App ---
void App_Alarm_Set_Loop(void);

// --- 闹钟后台检测 (放在 Clock_UpdateTime 里调用) ---
// 返回 true 表示闹钟响了
bool Tools_CheckAlarm(uint8_t current_h, uint8_t current_m);
void App_Alarm_Ring_Loop(void);
void Tools_SetAlarm(uint8_t h, uint8_t m, bool enable);
#endif