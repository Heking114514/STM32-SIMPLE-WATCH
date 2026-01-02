#include "app_mp3.h"
#include "main.h"
#include "oled.h"
#include "key.h"
#include "menu_core.h"
#include "mp3_player.h" // 包含你的驱动头文件
#include <stdio.h>

// ====================================================================
// 1. 图片资源 (使用你提供的字模)
// ====================================================================
const uint8_t pauseData[] = {
0x00, 0x00, 0x00, 0xf8, 0xf8, 0xf0, 0xf0, 0xe0, 0xc0, 
0xc0, 0x00, 0x00, 0x80, 0x80, 0x80, 0x8f, 0x87, 0x87, 0x83, 0x81, 0x81, 0x80, 0x80, 0x80, 
};
const Image pauseImg = {12, 15, pauseData};

const uint8_t leftData[] = {
0x00, 0x00, 0x00, 0x00, 0xf0, 0x00, 0x00, 0x00, 0x80, 0xc0, 0xc0, 0xe0, 0xf0, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x00, 
0x00, 0x03, 0x07, 0x07, 0x0f, 0x1f, 0x1f, 0x3f, 0x00, 0x00, 0x00, 0x00, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 
0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 
};
const Image leftImg = {18, 18, leftData};

const uint8_t rightData[] = {
0x00, 0x00, 0x00, 0x00, 0xf0, 0xe0, 0xe0, 0xc0, 0x80, 0x80, 0x00, 0x00, 0x00, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x3f, 
0x1f, 0x0f, 0x0f, 0x07, 0x03, 0x00, 0x00, 0x3f, 0x00, 0x00, 0x00, 0x00, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 
0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 
};
const Image rightImg = {18, 18, rightData};

const uint8_t playData[] = {
0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0xe0, 0x00, 0x00, 0x00, 0xf0, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x1f, 
0x00, 0x00, 0x00, 0x1f, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 
0xfc, 0xfc, 0xfc, 
};
const Image playImg = {17, 18, playData};

// ====================================================================
// 2. 变量与定义
// ====================================================================

// 布局坐标
#define ICON_Y_POS      44  
#define ICON_GAP        32  

// 光标枚举
enum {
    CURSOR_BACK = 0,
    CURSOR_PREV,
    CURSOR_PLAY,
    CURSOR_NEXT
};

// 状态变量
static uint8_t  cursor = CURSOR_PLAY;
static uint8_t  current_vol = 3;     // 默认音量
static uint8_t  is_vol_mode = 0;      // 0:控制模式, 1:音量模式
static uint16_t local_track_idx = 1;  // 本地显示的曲目号
static uint8_t  is_app_inited = 0;    // 初始化标志

// 引用 mp3_player.c 的全局变量
extern uint16_t MP3_TotalTracks; 

// ====================================================================
// 3. 应用程序主循环
// ====================================================================
void App_MP3_Loop(void) {
    // --- Step 0: 应用首次运行时，强制从第一首开始 ---
    if (!is_app_inited) {
        
        // 1. 【核心逻辑】软件状态复位
        local_track_idx = 1; // 强制将本地显示的曲目号设置为1
        cursor = CURSOR_PLAY;  // 光标复位到播放按钮
        
        // 2. 【核心逻辑】硬件状态复位
        MP3_PlayTrack(local_track_idx); // 强制命令硬件播放第一首歌
        HAL_Delay(10);
        MP3_SetVolume(current_vol);     // 设置初始音量
        
        // 3. 标记初始化完成
        is_app_inited = 1; 
    }

    // --- Step 1: 按键逻辑 ---
    
    // [模式A: 音量调节]
    if (is_vol_mode) {
        if (Key_IsSingleClick(KEY3_ID)) { // 增加音量
            if (current_vol <= 27) current_vol += 3;
            else current_vol = 30;
            MP3_SetVolume(current_vol);
        }
        if (Key_IsSingleClick(KEY1_ID)) { // 减小音量
            if (current_vol >= 3) current_vol -= 3;
            else current_vol = 0;
            MP3_SetVolume(current_vol);
        }
        if (Key_IsSingleClick(KEY2_ID)) { // 退出音量模式
            is_vol_mode = 0; 
        }
    }
    // [模式B: 播放控制]
    else {
        // KEY4: 进入音量设置
        if (Key_IsSingleClick(KEY4_ID)) {
            is_vol_mode = 1;
        }

        // 光标移动
        if (Key_IsSingleClick(KEY1_ID)) { 
            if (cursor > 0) cursor--; else cursor = CURSOR_NEXT;
        }
        if (Key_IsSingleClick(KEY3_ID)) { 
            if (cursor < CURSOR_NEXT) cursor++; else cursor = CURSOR_BACK;
        }

        // 确认操作
        if (Key_IsSingleClick(KEY2_ID)) {
            switch (cursor) {
                case CURSOR_BACK:
                    is_app_inited = 0; // 清除标志，下次进来重新同步
                    MP3_Stop(); 
                    // 【核心修改】
                    // 不再使用 Menu_NavigateBack()，因为它只对菜单页有效
                    // 对于APP，直接切换回菜单模式即可
                    Menu_SwitchToMenu(); 
                    
                    return; // 立即退出 APP 循环
                
                case CURSOR_PREV:
                    MP3_Prev(); // 调用接口：上一曲
                    // 为了UI流畅，本地立即更新数字，不等待查询
                    if (local_track_idx > 1) local_track_idx--;
                    else if (MP3_TotalTracks > 0) local_track_idx = MP3_TotalTracks;
                    break;

                case CURSOR_PLAY:
                    MP3_TogglePlayback(); // 调用接口：播放/暂停
                    break;

                case CURSOR_NEXT:
                    MP3_Next(); // 调用接口：下一曲
                    // 本地更新数字
                    if (MP3_TotalTracks > 0 && local_track_idx < MP3_TotalTracks) local_track_idx++;
                    else local_track_idx = 1;
                    break;
            }
        }
    }

    // --- Step 2: 界面绘制 ---
    OLED_NewFrame();

    // A. 顶部返回按钮
    if (cursor == CURSOR_BACK) {
        OLED_DrawFilledRectangle(0, 0, 14, 10, OLED_COLOR_NORMAL);
        OLED_PrintASCIIString(1, 1, "<<", &afont12x6, OLED_COLOR_REVERSED);
    } else {
        OLED_PrintASCIIString(1, 1, "<<", &afont12x6, OLED_COLOR_NORMAL);
    }

    // B. 左上角显示: 当前/总数
    // 直接使用 MP3_TotalTracks 全局变量
    char buf[20];
    sprintf(buf, "%03d / %03d", local_track_idx, MP3_TotalTracks);
    OLED_PrintASCIIString(20, 0, buf, &afont8x6, OLED_COLOR_NORMAL);

    // C. 屏幕中间: 当前歌曲编号(大字)
    sprintf(buf, "%03d", local_track_idx);
    // 居中计算: 128 - 3*12 = 92, 92/2 = 46
    OLED_PrintASCIIString(46, 20, buf, &afont24x12, OLED_COLOR_NORMAL);

    // D. 底部控制图标
    // 计算坐标
    uint8_t x_play = 55; // 中间图标X
    uint8_t x_prev = x_play - ICON_GAP;
    uint8_t x_next = x_play + ICON_GAP;
    
    uint8_t box_w = 22; // 选中框宽高
    uint8_t box_h = 22;
    uint8_t box_y = ICON_Y_POS - 2;

    // 1. 上一曲图标
    if (cursor == CURSOR_PREV) {
        OLED_DrawFilledRectangle(x_prev - 2, box_y, box_w, box_h, OLED_COLOR_NORMAL);
        OLED_DrawImage(x_prev, ICON_Y_POS, &leftImg, OLED_COLOR_REVERSED);
    } else {
        OLED_DrawImage(x_prev, ICON_Y_POS, &leftImg, OLED_COLOR_NORMAL);
    }

    // 2. 播放/暂停图标
    // 直接调用 MP3_IsPlaying() 接口获取状态
    uint8_t is_playing = MP3_IsPlaying();
    
    if (cursor == CURSOR_PLAY) {
        OLED_DrawFilledRectangle(x_play - 2, box_y, box_w, box_h, OLED_COLOR_NORMAL);
        if (is_playing) {
            // 正在播放 -> 显示暂停图标 (||)
            OLED_DrawImage(x_play, ICON_Y_POS, &playImg, OLED_COLOR_REVERSED);
        } else {
            // 暂停中 -> 显示播放图标 (>)
            OLED_DrawImage(x_play + 3, ICON_Y_POS + 1, &pauseImg, OLED_COLOR_REVERSED);
        }
    } else {
        if (is_playing) {
            OLED_DrawImage(x_play, ICON_Y_POS, &playImg, OLED_COLOR_NORMAL);
        } else {
            OLED_DrawImage(x_play + 3, ICON_Y_POS + 1, &pauseImg, OLED_COLOR_NORMAL);
        }
    }

    // 3. 下一曲图标
    if (cursor == CURSOR_NEXT) {
        OLED_DrawFilledRectangle(x_next - 2, box_y, box_w, box_h, OLED_COLOR_NORMAL);
        OLED_DrawImage(x_next, ICON_Y_POS, &rightImg, OLED_COLOR_REVERSED);
    } else {
        OLED_DrawImage(x_next, ICON_Y_POS, &rightImg, OLED_COLOR_NORMAL);
    }

    // E. 音量调节弹窗
    if (is_vol_mode) {
        // 清空弹窗区域
        OLED_DrawFilledRectangle(20, 20, 88, 24, OLED_COLOR_REVERSED); 
        OLED_DrawRectangle(20, 20, 88, 24, OLED_COLOR_NORMAL);

        // 显示 "音量:" (这里用了汉字，确保你字库里有这两个字，否则用 Vol)
        OLED_ShowGBK(24, 24, "音量", 16, OLED_COLOR_NORMAL);
        OLED_PrintASCIIString(56, 26, ":", &afont12x6, OLED_COLOR_NORMAL);
        
        // 进度条
        OLED_DrawRectangle(50, 26, 32, 10, OLED_COLOR_NORMAL);
        uint8_t bar_w = (current_vol * 30) / 30; // 30是最大音量
        if (bar_w > 0) OLED_DrawFilledRectangle(52, 28, bar_w, 6, OLED_COLOR_NORMAL);

        // 数字
        sprintf(buf, "%d", current_vol);
        OLED_PrintASCIIString(86, 26, buf, &afont12x6, OLED_COLOR_NORMAL);
    }

    OLED_ShowFrame();
}