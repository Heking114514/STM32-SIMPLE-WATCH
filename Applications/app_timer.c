#include "main.h"      // <--- 核心修复：必须包含 HAL 库和基础类型
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "app_timer.h" // 你的头文件 (原 app_tools.h?)
#include "oled.h"
#include "key.h"
#include "menu_core.h"
#include "clock.h"
#include "mp3_player.h" // 【新增】 引入MP3头文件
#include "sys_params.h" 
#include "sys_params.h" // 【新增】引入参数系统

// --- 补丁：防止 oled.h 没有正确声明这些外部变量 ---
// 如果你的 oled.h 里已经写了 extern const Font ...，这些可以删掉
// 但留着也不会报错，能确保编译通过
// extern const Font afont12x6;
// extern const Font afont16x8;
// extern const Font afont8x6;

// ============================================================================
//   工具 1: 闹钟 (Alarm)
// ============================================================================

static uint8_t g_temp_h = 7;
static uint8_t g_temp_m = 0;
static bool g_temp_enabled = false;
static uint8_t edit_idx = 0; // 0:Hour, 1:Min, 2:Toggle

// 闹钟设置界面

// 【新增】外部接口实现
void Tools_SetAlarm(uint8_t h, uint8_t m, bool enable) {
    g_temp_h = h;
    g_temp_m = m;
    g_temp_enabled = enable;
}

// 检查闹钟 (使用 g_alarm_... 变量)
bool Tools_CheckAlarm(uint8_t current_h, uint8_t current_m) {
    if (!g_temp_enabled) return false;
    static uint8_t last_min = 60;
    
    if (current_h == g_temp_h && current_m == g_temp_m) {
        if (last_min != current_m) {
            last_min = current_m;
            return true; 
        }
    } else {
        // 时间过去了，重置防抖变量，以便明天同一时间触发
        if (last_min != 60 && current_m != g_temp_m) {
             last_min = 60; 
        }
    }
    return false;
}
void App_Alarm_Set_Loop(void) {
    // 临时编辑变量
    static uint8_t temp_h = 0;
    static uint8_t temp_m = 0;
    static bool temp_enabled = false;
    
    static uint8_t edit_idx = 0; 
    static uint8_t inited = 0;

    // 1. 初始化：从全局参数结构体加载
    if (!inited) {
        temp_h = g_sys_params.alarm_h;
        temp_m = g_sys_params.alarm_m;
        temp_enabled = (bool)g_sys_params.alarm_on;
        edit_idx = 0;
        inited = 1;
    }
    // 绘制 UI
    OLED_NewFrame();
    // OLED_PrintASCIIString(0, 0, "Set Alarm", &afont12x6, OLED_COLOR_NORMAL);
    OLED_ShowGBK(0, 0, "设置闹钟", 16, OLED_COLOR_NORMAL);
    char buf[20];
    sprintf(buf, "%02d:%02d", temp_h, temp_m);
    
    // 居中显示时间
    OLED_PrintASCIIString(44, 24, buf, &afont16x8, OLED_COLOR_NORMAL);
    
    // 显示开关状态
    const char* status = temp_enabled ? "[ON]" : "[OFF]";
    OLED_PrintASCIIString(50, 48, (char*)status, &afont12x6, OLED_COLOR_NORMAL);

    // 绘制光标 (下划线)
    if (edit_idx < 2) {
        // 时间部分: 44 + idx*24
        uint8_t x = 44 + edit_idx * 24;
        OLED_DrawLine(x, 42, x + 16, 42, OLED_COLOR_NORMAL);
    } else {
        // 开关部分
        OLED_DrawLine(50, 60, 50 + 24, 60, OLED_COLOR_NORMAL);
    }

    OLED_ShowFrame();

    // 按键逻辑
    if (Key_IsSingleClick(KEY1_ID)) { // UP
        if (edit_idx == 0) temp_h = (temp_h + 1) % 24;
        if (edit_idx == 1) temp_m = (temp_m + 1) % 60;
        if (edit_idx == 2) temp_enabled = !temp_enabled;
    }
    if (Key_IsSingleClick(KEY3_ID)) { // DOWN
        if (edit_idx == 0) temp_h = (temp_h == 0) ? 23 : temp_h - 1;
        if (edit_idx == 1) temp_m = (temp_m == 0) ? 59 : temp_m - 1;
        if (edit_idx == 2) temp_enabled = !temp_enabled;
    }
    if (Key_IsSingleClick(KEY2_ID)) { // OK
        edit_idx++;
        if (edit_idx > 2) {
            // 【核心】保存逻辑
            // 1. 更新全局参数
            g_sys_params.alarm_h = temp_h;
            g_sys_params.alarm_m = temp_m;
            g_sys_params.alarm_on = temp_enabled ? 1 : 0;
            
            // 2. 写入Flash并应用 (Apply_Params 会调用 Tools_SetAlarm 更新 g_alarm_xxx)
            System_Params_Save();
            
            inited = 0; 
            Menu_SwitchToMenu(); 
        }
    }
}


// ============================================================================
//   工具 2: 秒表 (Stopwatch)
// ============================================================================

typedef enum {
    SW_STOPPED,
    SW_RUNNING
} StopwatchState;

// 光标位置定义
enum {
    SW_CURSOR_BACK = 0, // 左上角 <<
    SW_CURSOR_LEFT,     // 左下角 (Start/Lap)
    SW_CURSOR_RIGHT     // 右下角 (Reset/Stop)
};

static StopwatchState sw_state = SW_STOPPED;
static uint32_t start_tick = 0;
static uint32_t elapsed_ms = 0; // 累计毫秒数
static uint32_t record_ms[3] = {0}; // 最多记录3条
static uint8_t record_count = 0;
static uint8_t sw_cursor = SW_CURSOR_LEFT; // 新增：秒表界面光标

void App_Stopwatch_Loop(void) {
    // 1. 计算当前时间
    uint32_t current_display_ms = elapsed_ms;
    if (sw_state == SW_RUNNING) {
        current_display_ms += (HAL_GetTick() - start_tick);
    }
    
    // 2. 格式化时间
    uint16_t min = (current_display_ms / 60000) % 60;
    uint16_t sec = (current_display_ms / 1000) % 60;
    uint16_t ms  = (current_display_ms % 1000) / 10; 

    char main_buf[15];
    sprintf(main_buf, "%02d:%02d.%02d", min, sec, ms);

    // 3. 绘制 UI
    OLED_NewFrame();
    
    // 顶部栏：返回键
    if (sw_cursor == SW_CURSOR_BACK) {
        OLED_DrawFilledRectangle(0, 0, 16, 10, OLED_COLOR_NORMAL);
        OLED_PrintASCIIString(2, 1, "<<", &afont12x6, OLED_COLOR_REVERSED);
    } else {
        OLED_PrintASCIIString(2, 1, "<<", &afont12x6, OLED_COLOR_NORMAL);
    }
    // OLED_PrintASCIIString(40, 0, "Stopwatch", &afont12x6, OLED_COLOR_NORMAL);
    OLED_ShowGBK(40, 0, "计时器", 16, OLED_COLOR_NORMAL);

    // 大数字显示时间
    OLED_PrintASCIIString(34, 16, main_buf, &afont16x8, OLED_COLOR_NORMAL);
    
    // 记录列表
    for (int i = 0; i < record_count; i++) {
        uint16_t r_min = (record_ms[i] / 60000) % 60;
        uint16_t r_sec = (record_ms[i] / 1000) % 60;
        uint16_t r_ms  = (record_ms[i] % 1000) / 10;
        char sub_buf[20];
        sprintf(sub_buf, "#%d %02d:%02d.%02d", i+1, r_min, r_sec, r_ms);
        OLED_PrintASCIIString(10, 34 + i*10, sub_buf, &afont8x6, OLED_COLOR_NORMAL);
    }

    // 底部功能键
    OLED_DrawLine(0, 52, 128, 52, OLED_COLOR_NORMAL); 
    
    const char *txt_left;
    const char *txt_right;
    
    if (sw_state == SW_STOPPED) {
        txt_left = "Start";
        txt_right = "Reset";
    } else {
        txt_left = "Lap";
        txt_right = "Stop";
    }

    // 左下角按钮
    if (sw_cursor == SW_CURSOR_LEFT) {
        OLED_DrawFilledRectangle(0, 54, 34, 10, OLED_COLOR_NORMAL);
        OLED_PrintASCIIString(2, 55, (char*)txt_left, &afont8x6, OLED_COLOR_REVERSED);
    } else {
        OLED_PrintASCIIString(2, 55, (char*)txt_left, &afont8x6, OLED_COLOR_NORMAL);
    }

    // 右下角按钮
    if (sw_cursor == SW_CURSOR_RIGHT) {
        OLED_DrawFilledRectangle(98, 54, 30, 10, OLED_COLOR_NORMAL);
        OLED_PrintASCIIString(100, 55, (char*)txt_right, &afont8x6, OLED_COLOR_REVERSED);
    } else {
        OLED_PrintASCIIString(100, 55, (char*)txt_right, &afont8x6, OLED_COLOR_NORMAL);
    }
    
    OLED_ShowFrame();

    // 4. 按键处理
    if (Key_IsSingleClick(KEY1_ID)) { // 移动光标
        if (sw_cursor > 0) sw_cursor--;
        else sw_cursor = SW_CURSOR_RIGHT;
    }

    if (Key_IsSingleClick(KEY3_ID)) { // 移动光标
        if (sw_cursor < SW_CURSOR_RIGHT) sw_cursor++;
        else sw_cursor = SW_CURSOR_BACK;
    }

    if (Key_IsSingleClick(KEY2_ID)) { // 执行
        switch (sw_cursor) {
            case SW_CURSOR_BACK: 
                sw_state = SW_STOPPED;
                Menu_SwitchToMenu();
                break;

            case SW_CURSOR_LEFT: 
                if (sw_state == SW_STOPPED) {
                    start_tick = HAL_GetTick() - elapsed_ms; 
                    sw_state = SW_RUNNING;
                } else {
                    if (record_count < 3) {
                        record_ms[record_count] = current_display_ms;
                        record_count++;
                    } else {
                        record_ms[0] = record_ms[1];
                        record_ms[1] = record_ms[2];
                        record_ms[2] = current_display_ms;
                    }
                }
                break;

            case SW_CURSOR_RIGHT: 
                if (sw_state == SW_RUNNING) {
                    elapsed_ms = current_display_ms; 
                    sw_state = SW_STOPPED;
                } else {
                    elapsed_ms = 0;
                    record_count = 0;
                }
                break;
        }
    }
}

// 闹钟响铃界面
void App_Alarm_Ring_Loop(void) {
    static uint32_t flash_tick = 0;
    static bool flash_state = false;
    static uint8_t is_sound_started = 0;    
    if (is_sound_started == 0) {
        MP3_SetVolume(25); // 设置闹钟音量 (0-30)
        MP3_PlayTrack(34); // 【核心】播放第34首曲子
        is_sound_started = 1; // 标记已播放
    }

    // 2. 屏幕闪烁逻辑
    if (HAL_GetTick() - flash_tick >= 200) {
        flash_tick = HAL_GetTick();
        flash_state = !flash_state;
        OLED_SetColorMode(flash_state); 
    }

    OLED_NewFrame();
    if (flash_state) {
        // OLED_PrintASCIIString(30, 20, "WAKE UP!", &afont16x8, OLED_COLOR_REVERSED);
        OLED_ShowGBK(30, 20, "时间到", 16, OLED_COLOR_REVERSED);
    } else {
        // OLED_PrintASCIIString(30, 20, "WAKE UP!", &afont16x8, OLED_COLOR_NORMAL);
        OLED_ShowGBK(30, 20, "时间到", 16, OLED_COLOR_NORMAL);
    }
    // OLED_PrintASCIIString(10, 48, "Press Any Key", &afont12x6, OLED_COLOR_NORMAL);
    OLED_ShowGBK(10, 48, "按任意键", 16, OLED_COLOR_NORMAL);
    OLED_ShowFrame();

    if (Key_IsSingleClick(KEY1_ID) || Key_IsSingleClick(KEY2_ID) || Key_IsSingleClick(KEY3_ID)) {
        MP3_Stop();
        is_sound_started = 0;
        OLED_SetColorMode(0); // 恢复正常
        Menu_SwitchToMenu();
    }
}