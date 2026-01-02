#include "clock.h"
#include "stm32f1xx_hal.h" // 假设你用HAL库
#include <stdio.h>
#include <time.h> // 【必须引入】

extern RTC_HandleTypeDef hrtc; // CubeMX生成的RTC句柄

static Clock_Display_t display_cache;
static TimeFormat g_time_fmt = TIME_FMT_24H; // 默认24H
static const char* week_day_map[] = {"", "MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN"};
// --- 格式设置 ---

// --- 内部辅助：获取 RTC 原始计数值 (总秒数) ---
static uint32_t RTC_GetCounter(void) {
    // 读取高位和低位组合成32位 (F1特有操作)
    uint32_t high = READ_REG(hrtc.Instance->CNTH & RTC_CNTH_RTC_CNT);
    uint32_t low  = READ_REG(hrtc.Instance->CNTL & RTC_CNTL_RTC_CNT);
    return (high << 16) | low;
}

static void RTC_SetCounter(uint32_t cnt) {
    // 必须先开启备份域写保护并在进入配置模式 (HAL库封装了)
    __HAL_RTC_WRITEPROTECTION_DISABLE(&hrtc);
    
    // 进入配置模式
    SET_BIT(hrtc.Instance->CRL, RTC_CRL_CNF);
    
    // 写寄存器
    WRITE_REG(hrtc.Instance->CNTH, (cnt >> 16));
    WRITE_REG(hrtc.Instance->CNTL, (cnt & 0xFFFF));
    
    // 退出配置模式
    CLEAR_BIT(hrtc.Instance->CRL, RTC_CRL_CNF);
    
    // 等待操作完成
    while (!READ_BIT(hrtc.Instance->CRL, RTC_CRL_RTOFF)); 
    
    __HAL_RTC_WRITEPROTECTION_ENABLE(&hrtc);
}

void Clock_SetFormat(TimeFormat fmt) { g_time_fmt = fmt; }
TimeFormat Clock_GetFormat(void) { return g_time_fmt; }
void Clock_ToggleFormat(void) { 
    g_time_fmt = (g_time_fmt == TIME_FMT_24H) ? TIME_FMT_12H : TIME_FMT_24H; 
}

void Clock_UpdateTime(void) {
    // 1. 获取硬件计数器（总秒数，基于2000-01-01）
    uint32_t total_sec = RTC_GetCounter();
    
    // 2. 转换为标准 time_t (time.h通常基于1970，我们需要做个转换或者直接借用结构体)
    // 这里我们自定义基准：假设 count=0 代表 2000-01-01 00:00:00
    // 为了利用标准库 mktime/localtime，我们需要加上 2000年到1970年的秒数差
    // 2000-1970 ≈ 30年 ≈ 946684800 秒
    time_t raw_time = (time_t)total_sec + 946684800; 
    
    struct tm *info;
    info = localtime(&raw_time); // 转换为年月日时分秒

    // 3. 填充显示缓存
    // info->tm_year 是从1900算起的，所以 2023 就是 123
    uint16_t year = info->tm_year + 1900;
    uint8_t month = info->tm_mon + 1; // 0-11 -> 1-12
    uint8_t day   = info->tm_mday;
    uint8_t wday  = info->tm_wday; // 0=Sun
    uint8_t hour  = info->tm_hour;
    uint8_t min   = info->tm_min;
    uint8_t sec   = info->tm_sec;

    // 格式化日期
    sprintf(display_cache.date_str, "%04d-%02d-%02d", year, month, day);
    
    // 格式化星期
    if (wday < 7) strcpy(display_cache.week_str, week_day_map[wday]);

    // 格式化时间
    if (g_time_fmt == TIME_FMT_24H) {
        sprintf(display_cache.time_str, "%02d:%02d:%02d", hour, min, sec);
        display_cache.is_pm = false;
    } else {
        display_cache.is_pm = (hour >= 12);
        uint8_t h12 = hour;
        if (h12 == 0) h12 = 12;      
        else if (h12 > 12) h12 -= 12; 
        sprintf(display_cache.time_str, "%02d:%02d:%02d", h12, min, sec);
    }
}

// --- 读写数值 (这是对 HAL RTC 的封装) ---
void Clock_GetTimeValues(uint8_t *h, uint8_t *m, uint8_t *s) {
    uint32_t total_sec = RTC_GetCounter();
    time_t raw_time = (time_t)total_sec + 946684800;
    struct tm *info = localtime(&raw_time);
    *h = info->tm_hour; *m = info->tm_min; *s = info->tm_sec;
}

void Clock_GetDateValues(uint8_t *y, uint8_t *m, uint8_t *d) {
    uint32_t total_sec = RTC_GetCounter();
    time_t raw_time = (time_t)total_sec + 946684800;
    struct tm *info = localtime(&raw_time);
    // 这里要注意：你的设置APP如果是存2位的年份(23)，需要减去2000
    // info->tm_year 是 123 (代表2023)，所以 123 + 1900 - 2000 = 23
    *y = (info->tm_year + 1900) % 100; 
    *m = info->tm_mon + 1;
    *d = info->tm_mday;
}
void Clock_SetTime(uint8_t h, uint8_t m, uint8_t s) {
    // 1. 先读出当前完整时间（因为要保留日期不变）
    uint32_t current_sec = RTC_GetCounter();
    time_t raw_time = (time_t)current_sec + 946684800;
    struct tm *info = localtime(&raw_time);

    // 2. 修改时分秒
    info->tm_hour = h;
    info->tm_min  = m;
    info->tm_sec  = s;

    // 3. 重新计算总秒数 (mktime 会自动处理)
    time_t new_raw = mktime(info);
    uint32_t new_cnt = (uint32_t)(new_raw - 946684800);

    // 4. 写入硬件
    RTC_SetCounter(new_cnt);
}

void Clock_SetDate(uint8_t y, uint8_t m, uint8_t d) {
    // 1. 先读出当前完整时间（保留时分秒不变）
    uint32_t current_sec = RTC_GetCounter();
    time_t raw_time = (time_t)current_sec + 946684800;
    struct tm *info = localtime(&raw_time);

    // 2. 修改年月日
    // y 是 2位数的年份 (如 23)，tm_year 需要的是 "年份-1900"
    // 2023 -> tm_year = 123
    info->tm_year = (2000 + y) - 1900;
    info->tm_mon  = m - 1; // 1-12 -> 0-11
    info->tm_mday = d;

    // 3. 重新计算总秒数
    time_t new_raw = mktime(info);
    uint32_t new_cnt = (uint32_t)(new_raw - 946684800);

    // 4. 写入硬件
    RTC_SetCounter(new_cnt);
}

// --- 核心显示逻辑 ---
// void Clock_UpdateTime(void) {
//     RTC_TimeTypeDef sTime = {0};
//     RTC_DateTypeDef sDate = {0};

//     // HAL库读取时间和日期
//     HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
//     HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

//     // 格式化日期 (保持不变)
//     sprintf(display_cache.date_str, "20%02d-%02d-%02d", sDate.Year, sDate.Month, sDate.Date);

//     // 【新增】格式化星期
//     // sDate.WeekDay 的范围是 1(Mon) 到 7(Sun)
//     if (sDate.WeekDay > 0 && sDate.WeekDay <= 7) {
//         strcpy(display_cache.week_str, week_day_map[sDate.WeekDay]);
//     } else {
//         strcpy(display_cache.week_str, "---"); // 异常情况
//     }

//     // 格式化时间 (保持不变)
//     if (g_time_fmt == TIME_FMT_24H) {
//         sprintf(display_cache.time_str, "%02d:%02d:%02d", sTime.Hours, sTime.Minutes, sTime.Seconds);
//         display_cache.is_pm = false;
//     } else {
//         display_cache.is_pm = (sTime.Hours >= 12);
//         uint8_t h12 = sTime.Hours;
//         if (h12 == 0) h12 = 12;      
//         else if (h12 > 12) h12 -= 12; 
//         sprintf(display_cache.time_str, "%02d:%02d:%02d", h12, sTime.Minutes, sTime.Seconds);
//     }
// }

const Clock_Display_t* Clock_GetDisplayTime(void) {
    return &display_cache;
}

void Clock_Init(void) {
    Clock_UpdateTime();
}


static Alarm_t g_alarm = {0, 0, false, false}; // 默认00:00，关闭

Alarm_t* Clock_GetAlarm(void) { return &g_alarm; }

void Clock_CheckAlarm(void) {
    if (!g_alarm.enabled) return;
    
    // 获取当前时间 (h, m, s)
    uint8_t h, m, s;
    Clock_GetTimeValues(&h, &m, &s);
    
    // 简单的触发逻辑：时分匹配，且秒为0 (防止一分钟内重复触发)
    // 或者加上 triggered 标志位处理
    if (h == g_alarm.hour && m == g_alarm.min && s == 0) {
        g_alarm.triggered = true;
    }
}