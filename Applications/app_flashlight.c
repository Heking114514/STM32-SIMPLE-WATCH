#include "app_flashlight.h"
#include "main.h"       // 包含 LED_Pin, LED_GPIO_Port 的定义
#include "oled.h"
#include "key.h"
#include "menu_core.h"
#include "stm32f1xx_hal.h" // 【！！！添加这一行！！！】


// --- 内部状态变量 ---

// [核心] 手电筒的物理开关状态 (必须是 static，这样退出APP后状态会保留)
// 0 = OFF, 1 = ON
static uint8_t is_flashlight_on = 0; 

// 光标在界面上的位置
enum {
    CURSOR_FL_BACK = 0,
    CURSOR_FL_OFF,
    CURSOR_FL_ON
};
static uint8_t cursor = CURSOR_FL_OFF; // 默认光标停在"OFF"


void App_FlashlightLoop(void) {
    
    // --- 1. 按键逻辑处理 ---
    
    // 光标移动 (KEY1/KEY3)
    if (Key_IsSingleClick(KEY1_ID)) { // 上/左
        if (cursor > 0) cursor--;
        else cursor = CURSOR_FL_ON; // 循环
    }
    if (Key_IsSingleClick(KEY3_ID)) { // 下/右
        if (cursor < CURSOR_FL_ON) cursor++;
        else cursor = CURSOR_FL_BACK; // 循环
    }

    // 确认操作 (KEY2)
    if (Key_IsSingleClick(KEY2_ID)) {
        switch (cursor) {
            case CURSOR_FL_BACK:
                // 只退出APP，不改变手电筒的物理状态
                Menu_SwitchToMenu();
                return; // 立即退出

            case CURSOR_FL_OFF:
                // 如果当前灯是亮的，则执行关灯操作
                if (is_flashlight_on) {
                    is_flashlight_on = 0; // 更新软件状态
                    // 【关键操作】拉高 LED 引脚以关闭LED (假设低电平点亮)
                    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
                }
                break;

            case CURSOR_FL_ON:
                // 如果当前灯是灭的，则执行开灯操作
                if (!is_flashlight_on) {
                    is_flashlight_on = 1; // 更新软件状态
                    // 【关键操作】拉低 LED 引脚以点亮LED
                    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
                }
                break;
        }
    }

    // --- 2. 界面绘制 ---
    OLED_NewFrame();

    // A. 绘制返回按钮 "<<"
    if (cursor == CURSOR_FL_BACK) {
        OLED_DrawFilledRectangle(0, 0, 14, 10, OLED_COLOR_NORMAL);
        OLED_PrintASCIIString(1, 1, "<<", &afont12x6, OLED_COLOR_REVERSED);
    } else {
        OLED_PrintASCIIString(1, 1, "<<", &afont12x6, OLED_COLOR_NORMAL);
    }
    
    // B. 绘制 "OFF" 按钮
    // 居中 X 计算: (128 - (OFF宽度3*12 + ON宽度2*12 + 间距)) / 2
    uint8_t off_x = 28;
    uint8_t on_x = 76; 
    
    if (cursor == CURSOR_FL_OFF) {
        OLED_DrawFilledRectangle(off_x - 4, 20, 3*12 + 8, 28, OLED_COLOR_NORMAL);
        OLED_PrintASCIIString(off_x, 24, "OFF", &afont24x12, OLED_COLOR_REVERSED);
    } else {
        OLED_PrintASCIIString(off_x, 24, "OFF", &afont24x12, OLED_COLOR_NORMAL);
    }

    // C. 绘制 "ON" 按钮
    if (cursor == CURSOR_FL_ON) {
        OLED_DrawFilledRectangle(on_x - 4, 20, 2*12 + 8, 28, OLED_COLOR_NORMAL);
        OLED_PrintASCIIString(on_x, 24, "ON", &afont24x12, OLED_COLOR_REVERSED);
    } else {
        OLED_PrintASCIIString(on_x, 24, "ON", &afont24x12, OLED_COLOR_NORMAL);
    }

    // D. 绘制当前状态指示器 (下划线)
    if (is_flashlight_on) {
        // 灯亮时，下划线在 "ON" 下方
        OLED_DrawLine(on_x, 54, on_x + 2*12, 54, OLED_COLOR_NORMAL);
    } else {
        // 灯灭时，下划线在 "OFF" 下方
        OLED_DrawLine(off_x, 54, off_x + 3*12, 54, OLED_COLOR_NORMAL);
    }
    
    OLED_ShowFrame();
}