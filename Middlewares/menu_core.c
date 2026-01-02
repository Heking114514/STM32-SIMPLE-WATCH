#include "menu_core.h"
#include "oled.h"
#include "key.h"
#include "clock.h" // 仅用于时间更新
#include "app_timer.h" 
// 全局控制块
static MenuCtrl g_menu;

// 外部声明：在data.c中定义的初始化辅助函数
extern void Menu_LoadInitialState(MenuCtrl *ctrl);

static void Menu_Draw_PowerPopup(void) {
    // 保持背景不变（或者你可以先调 OLED_NewFrame() 清屏，看喜好）
    // 为了突出弹窗，这里我们用 NewFrame 清除背景，或者直接在原画面上盖一个框
    // 简单起见，我们清屏绘制，显得干净
    OLED_NewFrame(); 
    
    // 画一个大框
    OLED_DrawRectangle(10, 10, 108, 44, OLED_COLOR_NORMAL);
    
    // 标题 "Power Off?"
    // 居中：128 - (10字符*8) = 48 -> x=48/2=24 ?? 
    // "Power Off?" 长度10，font16x8宽8 -> 80px. 128-80=48, start=24
    OLED_PrintASCIIString(24, 16, "Power Off?", &afont16x8, OLED_COLOR_NORMAL);
    
    // 选项提示 (左:是, 右:否)
    // KEY1 在左边(物理上)，KEY3 在右边(物理上)
    
    // 左下角显示 "[YES]"
    OLED_DrawFilledRectangle(14, 38, 30, 12, OLED_COLOR_NORMAL);
    OLED_PrintASCIIString(16, 40, "YES", &afont8x6, OLED_COLOR_REVERSED); // 反色高亮
    
    // 右下角显示 "[NO]"
    OLED_DrawFilledRectangle(84, 38, 24, 12, OLED_COLOR_NORMAL);
    OLED_PrintASCIIString(88, 40, "NO", &afont8x6, OLED_COLOR_REVERSED);  // 反色高亮
    
    OLED_ShowFrame();
}


static void Menu_Process_PowerPopup(void) {
    
    // 左键 (KEY1) -> 确认关机
    if (Key_IsSingleClick(KEY1_ID)) {
        // --- 执行关机动作 ---
        
        // 1. 界面反馈
        OLED_NewFrame();
        OLED_ShowGBK(32, 24, "再见", 16, OLED_COLOR_NORMAL);
        OLED_ShowFrame();
        HAL_Delay(500);
        
        // 2. 关闭屏幕
        OLED_DisPlay_Off();
        
        // 3. 拉低 PB13 (关机引脚)
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
        
        // 4. 死循环，等待电源切断
        while(1) {
            // 可选：如果硬件设计是按下按键才能开机，这里不需要做任何事
            // 如果是软关机需要维持低电平，就在这卡住
        }
    }
    
    // 右键 (KEY3) -> 取消，返回上一界面
    if (Key_IsSingleClick(KEY3_ID)) {
        g_menu.mode = g_menu.last_mode; // 恢复之前的模式
        // 注意：不需要恢复画面，下一次 Loop 会自动重绘
    }
}

// 3. 检测 KEY4 长按 (全局检测)
static void Menu_Check_PowerKey(void) {
    static uint32_t press_start_tick = 0;
    static uint8_t is_pressing = 0;
    
    // 如果当前已经在弹窗了，就不检测长按了，避免重复触发
    if (g_menu.mode == SYS_MODE_POWER_POPUP) {
        // 如果松开按键，重置状态
        if (Key_GetRawState(KEY2_ID) == 1) {
            is_pressing = 0;
            press_start_tick = 0;
        }
        return;
    }

    // 检测 KEY4 物理状态 (0为按下)
    if (Key_GetRawState(KEY2_ID) == 0) {
        if (is_pressing == 0) {
            // 刚按下，记录时间
            is_pressing = 1;
            press_start_tick = HAL_GetTick();
        } 
        else {
            // 持续按下中，检查时间
            if ((HAL_GetTick() - press_start_tick) > 3000) { // > 5000ms
                // 触发！切换到弹窗模式
                g_menu.last_mode = g_menu.mode; // 备份当前模式
                g_menu.mode = SYS_MODE_POWER_POPUP;
                
                // 可选：为了防止松手时触发单击事件(KEY4原本是Menu键?)
                // 如果 KEY4 有其他短按功能，这里长按触发后应设法屏蔽短按
                // 由于 Key_IsSingleClick 也是基于状态机的，这里长按一直不松手，
                // 短按逻辑应该不会触发，或者我们可以手动清除一下
            }
        }
    } else {
        // 松开
        is_pressing = 0;
        press_start_tick = 0;
    }
}

// --- 辅助函数 ---
void Menu_SwitchToApp(AppLoopCallback app_func) {
    g_menu.mode = SYS_MODE_APP;
    g_menu.current_app = app_func;
    OLED_NewFrame(); // 清屏防止残影
}

void Menu_SwitchToMenu(void) {
    g_menu.mode = SYS_MODE_MENU;
    // 这里保持上次的 current_page，或者你可以重置为 Page_Main
}

// --- 菜单绘制引擎 ---
#define MAX_VISIBLE_LINES 4 // 屏幕能显示几行
void Menu_Draw_GenericList(void) {
    MenuPage *page = g_menu.current_page;
    OLED_NewFrame();
    
    // 检查是否有标题，如果有就画出来
    if (page->title) {
        // 标题用大一点的字库，居中
        uint8_t title_len = 0;
        // 简单判断是否包含中文（首字节 > 127）
        if ((uint8_t)page->title[0] > 0x80) {
            title_len = (strlen(page->title) / 3) * 16; // 假设每个汉字16px
        } else {
            title_len = strlen(page->title) * 6;
        }
        uint8_t title_x = (128 - title_len) / 2;
        if ((uint8_t)page->title[0] > 0x80) {
            OLED_ShowGBK(title_x, 0, (char*)page->title, 16, OLED_COLOR_NORMAL);
        } else {
            OLED_PrintASCIIString(title_x, 2, (char*)page->title, &afont12x6, OLED_COLOR_NORMAL);
        }
        OLED_DrawLine(0, 15, 128, 15, OLED_COLOR_NORMAL);
    }
    
    // 从标题下方开始绘制菜单项
    uint8_t start_y = page->title ? 16 : 0;
    uint8_t max_lines = (page->title) ? 3 : 4;

    for (uint8_t i = 0; i < MAX_VISIBLE_LINES; i++) {
        uint8_t item_idx = g_menu.view_offset + i;
        if (item_idx >= page->item_count) break;

        uint8_t y = start_y + i * 16; 
        const MenuItem *item = &page->items[item_idx];
        
        // 智能判断是中文还是英文
        uint8_t is_chinese = ((uint8_t)item->name[0] > 0x80);

        // 检查是否是选中状态
        if (item_idx == g_menu.cursor_pos) {
            OLED_DrawFilledRectangle(0, y, 128, 16, OLED_COLOR_NORMAL); // 统一反色背景
            
            if (is_chinese) {
                OLED_ShowGBK(2, y, (char*)item->name, 16, OLED_COLOR_REVERSED);
            } else {
                OLED_PrintASCIIString(2, y+2, (char*)item->name, &afont12x6, OLED_COLOR_REVERSED);
            }
        } else {
            // 未选中状态
            if (is_chinese) {
                OLED_ShowGBK(2, y, (char*)item->name, 16, OLED_COLOR_NORMAL);
            } else {
                OLED_PrintASCIIString(2, y+2, (char*)item->name, &afont12x6, OLED_COLOR_NORMAL);
            }
        }
    }
    OLED_ShowFrame();
}

// 在 Core/Src/menu_core.c 中

void Menu_Draw_Carousel(void) {
    MenuPage *page = g_menu.current_page;
    OLED_NewFrame();
    
    // --- 2. 绘制当前项的文字 (底部居中) - 【修改部分】 ---
    const MenuItem *center_item = &page->items[g_menu.cursor_pos];
    
    // 检查 name 指针是否有效，防止 NULL 导致 strlen 崩溃
    if (center_item->name != NULL) { 
        // 智能判断是中文还是英文 (UTF-8 编码的中文字符首字节 > 0x80)
        uint8_t is_chinese = ((uint8_t)center_item->name[0] > 0x80);
        uint8_t text_x;

        if (is_chinese) {
            // 【中文路径】
            // 计算居中坐标：假设每个汉字宽 16 像素
            uint8_t char_count = strlen(center_item->name) / 3; // UTF-8 一个汉字占3字节
            text_x = ((128 - char_count * 16) / 2) - 6;
            
            // 调用汉字显示函数
            OLED_ShowGBK(text_x, 48, (char*)center_item->name, 16, OLED_COLOR_NORMAL);
        } else {
            // 【英文路径】 (保持原逻辑)
            // 计算居中坐标：使用 afont12x6，每个字符宽 6 像素
            uint8_t name_len = strlen(center_item->name);
            text_x = (128 - name_len * 6) / 2; 
            
            // 调用 ASCII 显示函数
            OLED_PrintASCIIString(text_x, 54, (char*)center_item->name, &afont12x6, OLED_COLOR_NORMAL);
        }
    }
    // --- 【修改结束】 ---

    // 3. 计算索引
    int8_t idx_left  = (g_menu.cursor_pos - 1 + page->item_count) % page->item_count;
    int8_t idx_right = (g_menu.cursor_pos + 1) % page->item_count;
    int8_t idx_center = g_menu.cursor_pos;

    // --- 4. 坐标定义 ---
    const uint8_t IMG_W = 40;
    const uint8_t IMG_H = 40;
    uint8_t cx = 44;
    uint8_t cy = 6;
    uint8_t side_y_offset = 8; 

    // 绘制左侧图片
    if (page->item_count > 1) { 
        if (page->items[idx_left].icon) {
            OLED_DrawImage(2, cy + side_y_offset, page->items[idx_left].icon, OLED_COLOR_NORMAL);
        }
    }

    // 绘制右侧图片
    if (page->item_count > 1) {
        if (page->items[idx_right].icon) {
            OLED_DrawImage(86, cy + side_y_offset, page->items[idx_right].icon, OLED_COLOR_NORMAL);
        }
    }
    
    // 绘制中间图片 (覆盖两侧)
    OLED_DrawFilledRectangle(cx - 2, cy, IMG_W + 4, IMG_H, OLED_COLOR_REVERSED); 
    
    if (page->items[idx_center].icon) {
        OLED_DrawImage(cx, cy, page->items[idx_center].icon, OLED_COLOR_NORMAL);
    } else {
        // 无图替代框
        OLED_DrawRectangle(cx, cy, IMG_W, IMG_H, OLED_COLOR_NORMAL);
        OLED_PrintASCIIString(cx + 4, cy + 16, "NoIcon", &afont8x6, OLED_COLOR_NORMAL);
    }

    // --- 5. 绘制选中框 (四角) ---
    uint8_t bx = cx - 2;
    uint8_t by = cy - 2;
    uint8_t bw = IMG_W + 4;
    uint8_t bh = IMG_H + 4;
    uint8_t len = 6;

    // 左上角
    OLED_DrawLine(bx, by, bx + len, by, OLED_COLOR_NORMAL);
    OLED_DrawLine(bx, by, bx, by + len, OLED_COLOR_NORMAL);
    // 右上角
    OLED_DrawLine(bx + bw, by, bx + bw - len, by, OLED_COLOR_NORMAL);
    OLED_DrawLine(bx + bw, by, bx + bw, by + len, OLED_COLOR_NORMAL);
    // 左下角
    OLED_DrawLine(bx, by + bh, bx + len, by + bh, OLED_COLOR_NORMAL);
    OLED_DrawLine(bx, by + bh, bx, by + bh - len, OLED_COLOR_NORMAL);
    // 右下角
    OLED_DrawLine(bx + bw, by + bh, bx + bw - len, by + bh, OLED_COLOR_NORMAL);
    OLED_DrawLine(bx + bw, by + bh, bx + bw, by + bh - len, OLED_COLOR_NORMAL);

    OLED_ShowFrame();
}


// --- 菜单导航引擎 ---
void Menu_Process_Navigate(void) {
    MenuPage *page = g_menu.current_page;
    if (!page) return;

    // 根据是否有标题，动态决定一页能显示几行 (有标题3行，无标题4行)
    uint8_t max_lines = (page->title) ? 3 : 4;

    // =======================================================
    //  KEY3: Previous (向上 / 左 / 上一项)
    // =======================================================
    if (Key_IsSingleClick(KEY3_ID)) { 
        if (page->layout == LAYOUT_CAROUSEL) {
            // [轮播模式]：循环递减
            if (g_menu.cursor_pos == 0) g_menu.cursor_pos = page->item_count - 1;
            else g_menu.cursor_pos--;
        } else {
            // [列表模式]：普通递减
            if (g_menu.cursor_pos > 0) {
                // 1. 正常向上移动
                g_menu.cursor_pos--;
                // 2. 如果光标跑到了视图上方，视图跟上去 (滚动)
                if (g_menu.cursor_pos < g_menu.view_offset) {
                    g_menu.view_offset--;
                }
            } else {
                // 3. 在顶部继续按上 -> 循环到底部
                g_menu.cursor_pos = page->item_count - 1; 
                // 计算底部时的视图偏移
                if (page->item_count > max_lines) {
                    g_menu.view_offset = page->item_count - max_lines;
                } else {
                    g_menu.view_offset = 0;
                }
            }
        }
    }

    // =======================================================
    //  KEY1: Next (向下 / 右 / 下一项)
    // =======================================================
    if (Key_IsSingleClick(KEY1_ID)) { 
        if (page->layout == LAYOUT_CAROUSEL) {
             // [轮播模式]：循环递增
            g_menu.cursor_pos = (g_menu.cursor_pos + 1) % page->item_count;
        } else {
            // [列表模式]
            if (g_menu.cursor_pos < page->item_count - 1) {
                // 1. 正常向下移动
                g_menu.cursor_pos++;
                // 2. 如果光标跑到了视图下方，视图跟下来 (滚动)
                if (g_menu.cursor_pos >= g_menu.view_offset + max_lines) {
                    g_menu.view_offset++;
                }
            } else {
                // 3. 在底部继续按下 -> 循环到顶部
                g_menu.cursor_pos = 0; 
                g_menu.view_offset = 0;
            }
        }
    }

    // =======================================================
    //  KEY2: Enter (确认 / 进入)
    // =======================================================
    if (Key_IsSingleClick(KEY2_ID)) {
        const MenuItem *item = &page->items[g_menu.cursor_pos];
        
        if (item->next_page) {
            // 进入子菜单
            g_menu.current_page = item->next_page;
            g_menu.cursor_pos = 0;
            g_menu.view_offset = 0;
        } else if (item->run_app) {
            // 运行 APP
            Menu_SwitchToApp(item->run_app);
        }
    }
}

// --- 主循环 ---
static uint16_t last_tick = 0;

void Menu_Init(void) {
    Menu_LoadInitialState(&g_menu);
}

void Menu_Loop(void) {
    // 1. 全局时间更新 (保留你原来的逻辑)
    if (HAL_GetTick() - last_tick >= 1000) {
        last_tick = HAL_GetTick();
        Clock_UpdateTime();
    }
   uint8_t h, m, s;
    Clock_GetTimeValues(&h, &m, &s);
    
    // 检测闹钟是否触发
    // 注意：Tools_CheckAlarm 内部需要有防止1分钟内重复触发的逻辑(例如记录last_min)
    if (Tools_CheckAlarm(h, m)) {
        // 无论当前在看电子书、玩游戏还是在设置，强制跳转到响铃界面
        Menu_SwitchToApp(App_Alarm_Ring_Loop);
    }
    Menu_Check_PowerKey();
    // 2. 状态机分流
    switch (g_menu.mode) {
        case SYS_MODE_APP:
            // 运行当前的APP (主页、手电筒、MPU等)
            if (g_menu.current_app) g_menu.current_app();
            break;

        case SYS_MODE_MENU:
            Menu_Process_Navigate();
            // 根据布局选择绘制函数
            if (g_menu.current_page->layout == LAYOUT_CAROUSEL) {
                Menu_Draw_Carousel();
            } else {
                Menu_Draw_GenericList();
            }
            break;
        case SYS_MODE_POWER_POPUP: // 【新增状态处理】
            Menu_Process_PowerPopup();
            Menu_Draw_PowerPopup();
            break;
    }
}

/**
 * @brief 返回上一级菜单 (带光标记忆功能)
 */
void Menu_NavigateBack(void) {
    // 只有存在父级菜单时才返回
    if (g_menu.current_page && g_menu.current_page->parent) {
        
        MenuPage *child = g_menu.current_page;      // 当前页 (儿子)
        MenuPage *parent = child->parent;           // 上一级 (爸爸)

        // 1. 切换回父菜单
        g_menu.current_page = parent;

        // 2. [核心修改] 智能查找光标位置
        // 遍历父菜单的所有项，找到是谁把我们带进子菜单的
        g_menu.cursor_pos = 0; // 默认归零(防备忘录)
        
        for (uint8_t i = 0; i < parent->item_count; i++) {
            if (parent->items[i].next_page == child) {
                // 找到了！光标应该停在这里
                g_menu.cursor_pos = i; 
                break;
            }
        }
        uint8_t max_lines = (parent->title) ? 3 : 4;

        // 3. [核心修改] 修正滚动条位置 (view_offset)
        // 如果光标在很下面，必须把视图滚动下来，否则光标会跑出屏幕
       if (g_menu.cursor_pos >= max_lines) {
            g_menu.view_offset = g_menu.cursor_pos - max_lines + 1;
        } else {
            g_menu.view_offset = 0;
        }

        // 4. 切回菜单模式
        Menu_SwitchToMenu(); 
    }
}