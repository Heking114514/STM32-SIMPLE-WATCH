#include "menu_core.h"
#include "oled.h"
#include "key.h"
#include "clock.h"
#include "mpu6050.h"
#include <stdio.h>
#include <string.h>
#include "app_setting.h"
#include "app_timer.h"
#include "game.h" // 引用游戏模块
#include "battery.h" // <--- 新增：需要读取电量
#include "app_gradienter.h"
#include "app_mp3.h"
#include "app_flashlight.h"
#include "app_dino.h"
#include "app_about.h"
#include "app_message.h" // 记得包含头文件

// 引用外部图片 (防止未定义错误)
extern const Image Genshin_Impact; 
extern const Image musicImg;
extern const Image clockImg;
extern const Image balanceImg;
extern const Image messageImg;
extern const Image gameImg;
extern const Image settingImg;
extern const Image exitImg;
extern const Image mihayouImg;
extern const Image yuanshenImg;
extern const Image flashlightImg;
// --- 1. 声明菜单页 ---
extern MenuPage Page_Main;
extern MenuPage Page_Fellow;  


extern MenuPage Page_Setting;
extern MenuPage Page_DateTime;
extern MenuPage Page_TimeSet;
// --- 2. 定义应用程序 (APP) 逻辑 ---

// 声明
extern MenuPage Page_BadDay; // 这次我们把 BadDay 改造成一个菜单页
extern MenuPage Page_Game;



// [APP 1] 主页 (时钟界面)
static uint8_t home_cursor = 0; // 0:Flashlight, 1:Menu


static const MenuItem Items_Game[] = {
    {"<<",         NULL, NULL,           Menu_NavigateBack},
    {"俄罗斯方块",     NULL, NULL,           App_Tetris_Loop}, // 俄罗斯方块
    {"谷歌小恐龙",       NULL, NULL,           App_Dino_Loop},   // 恐龙快跑
};

// 列表布局的子菜单
MenuPage Page_Game = { "Games", Items_Game, 3, &Page_Main, LAYOUT_LIST };


// 3. Setting 主菜单 (Date & Time, 其他...)
static const MenuItem Items_Setting[] = {
    {"<<",          NULL, NULL,            Menu_NavigateBack},
    {"Date & Time", NULL, &Page_DateTime,  NULL},            // 进入DateTime子菜单
    {"Sleep Time",  NULL, NULL,            App_Set_Sleep_Loop}, // <--- 新增这一行
    {"System",      NULL, NULL,            App_System_Info_Loop},            // 预留
    {"Sound",       NULL, NULL,            App_Set_Sound_Loop},            // 预留
    {"Brightness",  NULL, NULL,            App_Set_Brightness_Loop}, // 亮度
};
MenuPage Page_Setting = { "Settings", Items_Setting, 6, &Page_Main, LAYOUT_LIST };

// 2. Date & Time 子菜单 (Date, Time)
static const MenuItem Items_DateTime[] = {
    {"<<",         NULL, NULL,           Menu_NavigateBack},
    {"Date",       NULL, NULL,           App_Set_Date_Loop},  // 进入日期设置APP
    {"Time",       NULL, &Page_TimeSet,  NULL},               // 进入TimeSet子菜单
};
MenuPage Page_DateTime = { "Date&Time", Items_DateTime, 3, &Page_Setting, LAYOUT_LIST };

static const MenuItem Items_TimeSet[] = {
    {"<<",         NULL, NULL,           Menu_NavigateBack},
    {"Set Value",  NULL, NULL,           App_Set_Time_Loop},  // 进入时间设置APP
    {"12H/24H",    NULL, NULL,           App_Toggle_Format},  // 切换格式
};
MenuPage Page_TimeSet = { "Time Opts", Items_TimeSet, 3, &Page_DateTime, LAYOUT_LIST };











/**
 * @brief 播放开机动画 (呼吸渐变切换)
 */
void Menu_PlayBootAnimation(void)
{
    // 1. 先把亮度设为 0 (全黑)，准备偷偷绘图
    OLED_SetBrightness(0);
    
    // ----------------- 第一张图 -----------------
    
    // 2. 绘制第一张图 (此时屏幕是黑的，用户看不见过程)
    OLED_NewFrame();
    OLED_DrawImage(0, 0, &mihayouImg, OLED_COLOR_NORMAL);
    OLED_ShowFrame();
    
    // 3. 渐亮 (Fade In)
    // 步长为5，速度适中。如果嫌慢就把 i+=5 改大，delay 改小
    for (int i = 0; i <= 255; i += 5) {
        OLED_SetBrightness(i);
        HAL_Delay(10); 
    }
    
    // 4. 保持高亮展示 1.5秒
    HAL_Delay(1500);
    
    // 5. 渐灭 (Fade Out)
    for (int i = 255; i >= 0; i -= 5) {
        OLED_SetBrightness(i);
        HAL_Delay(10);
    }
    
    // ----------------- 第二张图 -----------------
    
    // 6. 此时亮度为0，偷偷换成第二张图
    OLED_NewFrame();
    OLED_DrawImage(0, 0, &yuanshenImg, OLED_COLOR_NORMAL);
    OLED_ShowFrame();
    
    // 7. 再次渐亮 (Fade In)
    for (int i = 0; i <= 255; i += 5) {
        OLED_SetBrightness(i);
        HAL_Delay(10);
    }
    
    // 8. 保持展示 1.5秒
    HAL_Delay(1500);
    
    // 9. 动画结束，保持亮度为 255 给主界面使用
    OLED_SetBrightness(255);
}




// ============================================================================
//   [APP 1] 主页 (Home Screen) - 二级交互版
// ============================================================================
void App_Home_Loop(void) {
    // 状态变量：0=无焦点(默认), 1=菜单按钮有焦点
    static uint8_t home_focus = 0; 
    // 计时器：用于光标自动消失
    static uint32_t last_input_tick = 0;

    // --- 1. 逻辑处理 ---
    
    // 如果光标处于激活状态，且超过3秒无操作，自动取消光标
    if (home_focus == 1 && (HAL_GetTick() - last_input_tick > 3000)) {
        home_focus = 0; 
    }

    if (home_focus == 0) {
        // [状态 A: 无焦点]
        // 按下任意键 (KEY1/2/3)，仅仅是唤醒光标
        if (Key_IsSingleClick(KEY1_ID) || Key_IsSingleClick(KEY2_ID) || Key_IsSingleClick(KEY3_ID)) {
            home_focus = 1;
            last_input_tick = HAL_GetTick(); // 记录时间
        }
    } 
    else {
        // [状态 B: 已聚焦 "菜单"]
        // 只有按下 KEY2 (确认键) 才会进入菜单
        if (Key_IsSingleClick(KEY2_ID)) {
            home_focus = 0; // 重置状态，下次回来还是无焦点
            Menu_SwitchToMenu();
            return;
        }
        
        // 如果按了 KEY1 或 KEY3，更新一下时间，防止光标消失，但不做其他动作
        // (相当于维持高亮)
        if (Key_IsSingleClick(KEY1_ID) || Key_IsSingleClick(KEY3_ID)) {
            last_input_tick = HAL_GetTick();
        }
    }

    // --- 2. 界面绘制 ---
     OLED_NewFrame();
    const Clock_Display_t *dt = Clock_GetDisplayTime();
    int battery_pct = Battery_Get_Percentage();

    // ==========================================================
    // A. 中央时间 (【核心修改部分】)
    // ==========================================================
    if (Clock_GetFormat() == TIME_FMT_24H) {
        // [24H 模式]：保持原来的居中显示
        // X = (128 - 96) / 2 = 16
        OLED_PrintASCIIString(16, 20, (char*)dt->time_str, &afont24x12, OLED_COLOR_NORMAL);
    } 
    else {
        // [12H 模式]：时间左移，右边放 AM/PM
        // 策略：时间放 X=4, AM/PM 放 X=104 (留4像素间隔)
        
        // 1. 画时间 (8个字符 * 12像素宽 = 96)
        OLED_PrintASCIIString(4, 20, (char*)dt->time_str, &afont24x12, OLED_COLOR_NORMAL);
        
        // 2. 画 AM/PM (使用中等字体 afont12x6)
        // 字体高12，为了和大字(高24)的底部对齐，Y坐标需要计算：
        // 大字底部 Y = 20 + 24 = 44
        // AM/PM Y = 44 - 12 = 32
        const char* ampm = dt->is_pm ? "PM" : "AM";
        OLED_PrintASCIIString(104, 32, (char*)ampm, &afont12x6, OLED_COLOR_NORMAL);
    }

    // B. 右上角电量
    char bat_buf[5];
    sprintf(bat_buf, "%d%%", battery_pct);
    OLED_PrintASCIIString(0, 54, bat_buf, &afont8x6, OLED_COLOR_NORMAL);
    
    uint8_t bat_x = 24, bat_y = 54, bat_w = 12, bat_h = 6;
    OLED_DrawRectangle(bat_x, bat_y, bat_w, bat_h, OLED_COLOR_NORMAL);
    OLED_DrawLine(bat_x + bat_w, bat_y + 1, bat_x + bat_w, bat_y + bat_h - 2, OLED_COLOR_NORMAL);
    if (battery_pct > 0) {
        uint8_t fill_w = (battery_pct * (bat_w - 2)) / 100;
        if (fill_w == 0) fill_w = 1;
        OLED_DrawFilledRectangle(bat_x + 1, bat_y + 1, fill_w, bat_h - 2, OLED_COLOR_NORMAL);
    }

    // C. 左下角日期
    OLED_PrintASCIIString(10, 0, (char*)dt->date_str, &afont12x6, OLED_COLOR_NORMAL);
    OLED_PrintASCIIString(100, 0, (char*)dt->week_str, &afont12x6, OLED_COLOR_NORMAL);

    // 2. 【新增】显示星期 (星期一)
    // 放在日期旁边
    // 日期占 10*6 = 60 像素，留 4 像素间隔
    // D. 右下角：“菜单” (根据 focus 状态改变显示样式)
    if (home_focus == 1) {
        // [有焦点]：白底黑字 (反色)
        OLED_DrawFilledRectangle(96, 48, 32, 16, OLED_COLOR_NORMAL); // 画白框
        OLED_ShowGBK(96, 48, "菜单", 16, OLED_COLOR_REVERSED); // 写黑字
    } else {
        // [无焦点]：黑底白字 (正常)
        // OLED_DrawFilledRectangle(96, 48, 32, 16, OLED_COLOR_REVERSED); // 可选：清空区域
        OLED_ShowGBK(96, 48, "菜单", 16, OLED_COLOR_NORMAL); // 写白字
    }

    OLED_ShowFrame();
}

// [APP 2] 手电筒
void App_Flashlight_Loop(void) {
    if (Key_IsSingleClick(KEY1_ID) || Key_IsSingleClick(KEY2_ID) || Key_IsSingleClick(KEY3_ID)) {
        Menu_SwitchToApp(App_Home_Loop); // 返回主页
        return;
    }
    extern uint8_t OLED_GRAM[8][128];
    memset(OLED_GRAM, 0xFF, sizeof(OLED_GRAM)); // 全白
    OLED_ShowFrame();
}

// [APP 3] MPU6050 显示
void App_MPU6050_Loop(void) {
    if (Key_IsSingleClick(KEY2_ID)) {
        Menu_SwitchToMenu(); // 返回菜单
        return;
    }
    
    const MPU6050_t* mpu = MPU6050_GetDataPtr();
    char str[3][20];
    sprintf(str[0], "Roll : %.2f", mpu->KalmanAngleX);
    sprintf(str[1], "Pitch: %.2f", mpu->KalmanAngleY);
    sprintf(str[2], "Yaw  : %.2f", mpu->KalmanAngleZ);

    OLED_NewFrame();
    OLED_PrintASCIIString(0, 0,  "MPU6050 Angles", &afont12x6, OLED_COLOR_NORMAL);
    for(int i=0; i<3; i++) OLED_PrintASCIIString(0, 16 + i*16, str[i], &afont12x6, OLED_COLOR_NORMAL);
    OLED_ShowFrame();
}

// [APP 4] Bad Day (看图)
void App_BadDay_Loop(void) {
    if (Key_IsSingleClick(KEY2_ID)) {
        Menu_SwitchToMenu(); // 返回菜单
        return;
    }
    OLED_NewFrame();
    OLED_DrawImage(0, 0, &Genshin_Impact, OLED_COLOR_NORMAL);
    OLED_ShowFrame();
}

// [APP 5] 通用详情页 (演示用)
void App_Detail_Generic(void) {
    if (Key_IsSingleClick(KEY2_ID)) { Menu_SwitchToMenu(); return; }
    OLED_NewFrame();
    OLED_PrintASCIIString(0, 20, "Under Dev...", &afont16x8, OLED_COLOR_NORMAL);
    OLED_PrintASCIIString(0, 40, "Press OK Back", &afont12x6, OLED_COLOR_NORMAL);
    OLED_ShowFrame();
}


// --- 3. 定义菜单结构 (数据驱动的核心) ---

// 主菜单的内容
static const MenuItem Items_Main[] = {
    // 名字           // 图标指针       // 子菜单       // APP
    {"退出",           &exitImg,    NULL,          App_Home_Loop},    // 第一项是返回
    {"水平仪",      &balanceImg,       NULL,          App_Gradienter_Loop},
    {"闹钟",      &clockImg,       &Page_BadDay,           NULL},     
    {"信息",       &messageImg,    &Page_Fellow,  NULL},             // 进子菜单
    {"音乐",        &musicImg,     NULL,          App_MP3_Loop},
    {"设置",      &settingImg,     &Page_Setting,          NULL},
    {"游戏",         &gameImg,        &Page_Game,    NULL},               // 修改这一行：指向子菜单
    {"手电筒",       &flashlightImg, NULL,          App_FlashlightLoop}, 
};

// 定义 Fellow 的子菜单项
static const MenuItem Items_Fellow[] = {
    // 列表模式下 icon 可以填 NULL
    {"<<",       NULL, NULL,              Menu_NavigateBack}, 
    {"作者", NULL, NULL,              App_About_Loop},
    {"关于我", NULL, NULL,              App_Message_Loop},
};



// 构建主菜单页
MenuPage Page_Main = { 
    .title = "Main Menu", 
    .items = Items_Main, 
    .item_count = 8, 
    .parent = NULL,
    .layout = LAYOUT_CAROUSEL
};

// 定义 Fellow 页面 (注意 parent 指向 Page_Main)
MenuPage Page_Fellow = { 
    "Fellow Menu",
     Items_Fellow, 3, 
     &Page_Main 
};
// --- 初始化入口 ---
// 供 core.c 调用，设置初始状态


// 定义子菜单项
static const MenuItem Items_BadDay[] = {
    {"<<",         NULL, NULL,           Menu_NavigateBack},
    {"闹钟",      NULL, NULL,           App_Alarm_Set_Loop},    // 闹钟
    {"计时器",  NULL, NULL,           App_Stopwatch_Loop},    // 秒表
};

// 定义页面 (列表布局)
MenuPage Page_BadDay = { "Tools Box", Items_BadDay, 3, &Page_Main, LAYOUT_LIST };


void Menu_LoadInitialState(MenuCtrl *ctrl) {
    ctrl->mode = SYS_MODE_APP;
    ctrl->current_app = App_Home_Loop; // 默认进主页
    ctrl->current_page = &Page_Main;   // 默认菜单是Main
}