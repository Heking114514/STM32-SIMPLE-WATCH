#include "mp3_test.h"
#include "mp3_player.h" // 底层MP3驱动
#include "key.h"        // 按键驱动
#include "oled.h"       // OLED驱动
#include "battery.h"    // 电池检测模块
#include <stdio.h>

// 模块内部使用的静态变量，用于跟踪状态
static uint8_t current_volume = 20; // 初始音量

/**
 * @brief  初始化MP3播放器应用
 */
void MP3_App_Init(void)
{
    // 1. 初始化底层MP3模块
    MP3_Init_Async();
    
    // 2. 设置一个合适的初始音量
    MP3_SetVolume(current_volume);
    
    // 3. (可选) 开机自动播放第一首歌
    // MP3_PlayTrack(1); 
}


/**
 * @brief  MP3播放器应用的循环处理任务
 */
void MP3_App_Loop(void)
{
    uint8_t key_value;
    char oled_buffer[32];

    // --- 1. 按键处理 ---
    key_value = Key_Scan(0); // 扫描按键

    if (key_value != KEY_NONE)
    {
        switch (key_value)
        {
            case KEY1_PRES: // 上一曲
                MP3_Prev();
                break;
            
            case KEY2_PRES: // 切换 播放/暂停
                MP3_TogglePlayback();
                break;
            
            case KEY3_PRES: // 下一曲
                MP3_Next();
                break;

            case KEY4_PRES: // 音量循环增加
                current_volume += 5;
                if (current_volume > 30) {
                    current_volume = 5;
                }
                MP3_SetVolume(current_volume);
                break;
        }
    }

    // --- 2. MP3 后台任务 (处理播放完成等事件) ---
    MP3_Loop_Handler();

    // --- 3. OLED 屏幕刷新 ---
    OLED_NewFrame();

    // 显示电池电量
    int percentage = Battery_Get_Percentage();
    sprintf(oled_buffer, "Battery: %d%%", percentage);
    OLED_PrintASCIIString(0, 0, oled_buffer, &afont16x8, OLED_COLOR_NORMAL);

    // 显示播放状态
    if (MP3_IsPlaying()) {
        sprintf(oled_buffer, "Status: Playing");
    } else {
        sprintf(oled_buffer, "Status: Paused/Stopped");
    }
    OLED_PrintASCIIString(0, 16, oled_buffer, &afont16x8, OLED_COLOR_NORMAL);

    // 显示音量
    sprintf(oled_buffer, "Volume: %d/30", current_volume);
    OLED_PrintASCIIString(0, 32, oled_buffer, &afont16x8, OLED_COLOR_NORMAL);

    OLED_ShowFrame();
}