#ifndef __APP_POWER_H
#define __APP_POWER_H

#include <stdint.h>
#include <stdbool.h>

// --- 常量定义 ---
// 你可以定义一些预设档位，方便 UI 使用
#define SLEEP_TIME_10S    10000
#define SLEEP_TIME_30S    30000
#define SLEEP_TIME_60S    60000
#define SLEEP_TIME_NEVER  0      // 0 代表永不熄屏

// --- 接口函数 ---

void Power_Init(void);
bool Power_Update(void);
void Power_ResetTimer(void);

// 【新增】 设置息屏时间 (单位: ms, 0表示不息屏)
void Power_SetTimeout(uint32_t ms);

// 【新增】 获取当前设置的时间 (用于在设置界面回显)
uint32_t Power_GetTimeout(void);

#endif