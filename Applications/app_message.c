#include "app_message.h"
#include "main.h"
#include "oled.h"
#include "key.h"
#include "menu_core.h"
#include "w25qxx.h"
#include <string.h> // 需要 memset, strlen

// ============================================================================
//   配置区域
// ============================================================================
// 倒数第3个扇区地址: 16MB - 12KB = 0x1000000 - 0x3000
#define MANUAL_FLASH_ADDR   0x00FFD000 
#define BYTES_PER_LINE      32  // 每行预留32字节 (屏幕一行最多显示16字节内容，留余量)

// ============================================================================
//   【烧录区域】 (烧录完成后，可以将下方数组注释掉以节省单片机 Flash)
// ============================================================================
// 如果你已经烧录过了，可以将下面的 #define ENABLE_BURN_CODE 注释掉
// #define ENABLE_BURN_CODE 

#ifdef ENABLE_BURN_CODE
static const char *raw_text_data[] = {
    "【使用说明书】",
    " ",
    "1. 产品简介",
    "感谢您选择HJH",
    "开发的智能手表",
    "本设备基于F103",
    "高性能芯片打造",
    "集成了多项实用",
    "功能与娱乐组件",
    "旨在提供极致的",
    "嵌入式交互体验",
    " ",
    "2. 硬件配置",
    "主控：f103c8t6",
    "屏幕：1.3' OLED",
    "存储：16M Flash",
    "姿态：MPU6050",
    "音频：YX5200",
    "电池：250mAh",
    " ",
    "3. 交互逻辑",
    "[基础操作]",
    "KEY1(上/左)：",
    "  菜单上移",
    "  音量减小",
    "  文本上滑",
    " ",
    "KEY3(下/右)：",
    "  菜单下移",
    "  音量增加",
    "  文本下滑",
    " ",
    "KEY2(确认)：",
    "  进入菜单",
    "  确认选项",
    "  暂停/播放",
    "  长按5秒关机",
    " ",
    "KEY4(开机)：",
    "  短按开机",
    "  弹出音量调节",
    " ",
    "4. 功能详解",
    "[体感唤醒]",
    "内置姿态解算",
    "抬腕自动亮屏",
    "放下自动息屏",
    "平放静止休眠",
    " ",
    "[音乐播放]",
    "支持TF卡读取",
    "高保真解码",
    "具备静音锁死",
    "防止误触播放",
    " ",
    "[系统设置]",
    "支持亮度调节",
    "息屏时间设定",
    "闹钟定时设置",
    "参数掉电保存",
    " ",
    "[实用工具]",
    "高亮手电筒",
    "六轴水平仪",
    "精确秒表计时",
    " ",
    "[游戏娱乐]",
    "移植谷歌恐龙",
    "经典方块游戏",
    "60帧流畅运行",
    " ",
    "5. 注意事项",
    "① 本机不防水",
    "   请远离水源",
    "② 请用5V充电",
    "   严禁高压",
    "③ 若系统死机",
    "   请按复位键",
    " ",
    "6. 开发者说",
    "嵌入式开发之路",
    "充满了挑战乐趣",
    "代码是我的诗篇",
    "硬件是我的画笔",
    "希望这个小作品",
    "能带给你启发",
    " ",
    "版本：V3.0 Final",
    "日期：2025-12",
    "作者：HJH",
    "Design For You",
    " ",
    "~~ 到底了 ~~",
    NULL // 结束标志
};


void Message_Burn_Text(void) {
    uint16_t i = 0;
    uint8_t buffer[BYTES_PER_LINE];
    uint32_t write_addr = MANUAL_FLASH_ADDR;

    // 1. 擦除扇区 (4KB，足够存 4096/32 = 128行)
    // 如果你的说明书超过128行，需要擦除连续的两个扇区
    W25Q_Erase_Sector(MANUAL_FLASH_ADDR);
    
    // 2. 循环写入
    while (raw_text_data[i] != NULL) {
        // 清空 buffer
        memset(buffer, 0, BYTES_PER_LINE);
        // 复制字符串到 buffer (防止溢出)
        strncpy((char*)buffer, raw_text_data[i], BYTES_PER_LINE - 1);
        
        // 写入 Flash
        W25Q_Write_NoCheck(buffer, write_addr, BYTES_PER_LINE);
        
        // 地址偏移
        write_addr += BYTES_PER_LINE;
        i++;
    }
    
    // 3. 写入结束标记 (写入一个空字符串或者特定标记)
    memset(buffer, 0, BYTES_PER_LINE);
    W25Q_Write_NoCheck(buffer, write_addr, BYTES_PER_LINE);
}
#else
// 如果注释了宏，保留一个空函数防止编译报错
void Message_Burn_Text(void) {}
#endif

// ============================================================================
//   阅读器逻辑 (从 Flash 读取)
// ============================================================================

#define LINE_H      16  
#define VISIBLE_ROWS 3  

static uint16_t scroll_line = 0; 
static uint16_t total_lines = 0; 

// 辅助函数：计算 Flash 里的文本总行数
static void Count_Total_Lines(void) {
    uint8_t buf[2]; // 只读前两个字节判断是否为空
    total_lines = 0;
    uint32_t addr = MANUAL_FLASH_ADDR;
    
    while (1) {
        W25Q_Read(buf, addr, 2);
        // 如果读到 0x00 (结束符) 或 0xFF (空白Flash)，则停止
        if (buf[0] == 0x00 || buf[0] == 0xFF) {
            break;
        }
        total_lines++;
        addr += BYTES_PER_LINE;
        
        // 安全限制：防止死循环 (假设最大200行)
        if (total_lines > 200) break;
    }
}

void App_Message_Loop(void) {
    // 1. 初始化：计算总行数 (仅第一次)
    if (total_lines == 0) {
        Count_Total_Lines();
    }

    // 2. 绘制界面
    OLED_NewFrame();
    
    // 标题栏
    OLED_DrawFilledRectangle(0, 0, 128, 16, OLED_COLOR_NORMAL);
    OLED_PrintASCIIString(2, 2, "<<", &afont8x6, OLED_COLOR_REVERSED);
    OLED_PrintASCIIString(32, 2, "User Manual", &afont12x6, OLED_COLOR_REVERSED);
    
    // 内容区
    uint8_t start_y = 16;
    // 临时缓冲区，用于从 Flash 读取一行文本
    char line_buf[BYTES_PER_LINE + 1]; 
    
    for (int i = 0; i < VISIBLE_ROWS; i++) {
        uint16_t current_idx = scroll_line + i;
        
        if (current_idx >= total_lines) break;
        
        // 计算 Flash 地址
        uint32_t read_addr = MANUAL_FLASH_ADDR + (current_idx * BYTES_PER_LINE);
        
        // 【核心】从 Flash 读取文本
        W25Q_Read((uint8_t*)line_buf, read_addr, BYTES_PER_LINE);
        line_buf[BYTES_PER_LINE] = '\0'; // 确保字符串结束符
        
        uint8_t y = start_y + i * LINE_H;
        
        // 显示 (line_buf 里的内容应该是 GBK 编码)
        OLED_ShowGBK(0, y, line_buf, 16, OLED_COLOR_NORMAL);
    }

    // 滚动条
    if (total_lines > VISIBLE_ROWS) {
        OLED_DrawLine(127, 16, 127, 63, OLED_COLOR_NORMAL);
        uint8_t bar_h = 48 * VISIBLE_ROWS / total_lines;
        if (bar_h < 4) bar_h = 4;
        uint16_t max_scroll = total_lines - VISIBLE_ROWS;
        uint8_t bar_y = 16 + (scroll_line * (48 - bar_h) / max_scroll);
        OLED_DrawFilledRectangle(125, bar_y, 3, bar_h, OLED_COLOR_NORMAL);
    }

    OLED_ShowFrame();

    // 3. 按键处理
    if (Key_IsSingleClick(KEY3_ID)) { // UP
        if (scroll_line > 0) scroll_line--;
    }
    
    if (Key_IsSingleClick(KEY1_ID)) { // DOWN
        if (scroll_line < total_lines - VISIBLE_ROWS) scroll_line++;
    }
    
    if (Key_IsSingleClick(KEY2_ID)) { // OK
        Menu_SwitchToMenu();
    }
}