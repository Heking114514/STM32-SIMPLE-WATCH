#ifndef __CLOCK_H
#define __CLOCK_H

#include <stdint.h>
#include <stdbool.h> // 需要 bool 类型

// 时间格式枚举
typedef enum {
    TIME_FMT_24H = 0,
    TIME_FMT_12H = 1
} TimeFormat;

// 闹钟结构体
typedef struct {
    uint8_t hour;
    uint8_t min;
    bool enabled; // 开关
    bool triggered; // 是否正在响铃
} Alarm_t;

// 用于显示的数据结构
typedef struct {
    char time_str[10]; // "12:30:45"
    char date_str[12]; // "2023-10-01"
    char week_str[10]; // 【新增】用于存储 "星期一"
    bool is_pm;        // 如果是12小时制，true代表下午
} Clock_Display_t;

// --- 基础功能 ---
void Clock_Init(void);
void Clock_UpdateTime(void); // 在主循环调用，更新缓存
const Clock_Display_t* Clock_GetDisplayTime(void);

// --- 设置接口 (供 Setting App 调用) ---
void Clock_SetFormat(TimeFormat fmt);
TimeFormat Clock_GetFormat(void);
void Clock_ToggleFormat(void); // 快捷切换

// 读写数值接口 (不仅是显示字符串，App需要拿到数字进行加减)
void Clock_GetTimeValues(uint8_t *h, uint8_t *m, uint8_t *s);
void Clock_GetDateValues(uint8_t *y, uint8_t *m, uint8_t *d);
void Clock_SetTime(uint8_t h, uint8_t m, uint8_t s);
void Clock_SetDate(uint8_t y, uint8_t m, uint8_t d);

#endif