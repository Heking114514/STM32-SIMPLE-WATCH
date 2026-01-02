#ifndef __MENU_CORE_H
#define __MENU_CORE_H

#include <stdint.h>
#include "oled.h" // 需要引用 Image 结构体定义
// --- 类型定义 ---
void Menu_NavigateBack(void);
// 应用程序循环函数指针 (用于主页、手电筒、MPU显示等独立界面)
typedef void (*AppLoopCallback)(void);

// 前向声明
struct MenuPage;


typedef enum {
    LAYOUT_LIST,    // 普通垂直列表
    LAYOUT_CAROUSEL // 图片轮播 (Cover Flow)
} PageLayout;

// 菜单项结构体
typedef struct {
    const char *name;           // 菜单项名字
    const Image *icon;
    struct MenuPage *next_page; // 子菜单指针 (如果有，按确认进入)
    AppLoopCallback run_app;    // 应用程序指针 (如果有，按确认运行)
} MenuItem;

// 菜单页结构体
typedef struct MenuPage {
    const char *title;          // 标题
    const MenuItem *items;      // 菜单项数组
    uint8_t item_count;         // 数量
    struct MenuPage *parent;    // 父菜单 (用于返回)
    PageLayout layout; 
} MenuPage;

// --- 系统模式枚举 ---
typedef enum {
    SYS_MODE_MENU,  // 正在浏览通用菜单
    SYS_MODE_APP,    // 正在运行独立APP (主页、手电筒、看图等)
    SYS_MODE_POWER_POPUP,  // 【新增】关机确认弹窗模式
} SystemMode;

// --- 核心控制块 ---
typedef struct {
    SystemMode mode;            // 当前模式
    MenuPage *current_page;     // [菜单模式] 当前菜单页
    uint8_t cursor_pos;         // [菜单模式] 光标位置
    uint8_t view_offset;        // [菜单模式] 滚动偏移
    
    AppLoopCallback current_app;// [APP模式] 当前运行的APP函数
    SystemMode last_mode;       

} MenuCtrl;

// --- 外部接口 ---
void Menu_Init(void);
void Menu_Loop(void);

// 供 data.c 调用的切换函数
void Menu_SwitchToApp(AppLoopCallback app_func);
void Menu_SwitchToMenu(void);
void Menu_NavigateBack(void);
void Menu_PlayBootAnimation(void);
#endif