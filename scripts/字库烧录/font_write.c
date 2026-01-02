#include "font_write.h"
#include "oled.h"
#include "w25qxx.h"
#include "stdio.h"  // for sprintf

// ！！！ 只有这里才能包含 burn_data.h ！！！
#include "burn_data.h" 

void write_process (void)
{
    
    char lcd_buf[32];

    // ================= 核心烧录逻辑 =================
    OLED_PrintASCIIString(0, 0,  "Waiting to prepare", &afont16x8, 1);
    OLED_ShowFrame();
    HAL_Delay(10000);
    OLED_NewFrame();
    #if BURN_STEP == 0
        // ---------------- [模式 0: 全片擦除] ----------------
        OLED_PrintASCIIString(0, 0,  "Mode: ERASE CHIP", &afont16x8, 1);
        OLED_PrintASCIIString(0, 16, "Please Wait...",   &afont16x8, 1);
        OLED_ShowFrame();

        // 执行全片擦除 (耗时 20s - 40s)
        W25Q_Erase_Chip(); 

        // 完成提示
        OLED_NewFrame();
        OLED_PrintASCIIString(0, 0,  "Mode: ERASE CHIP", &afont16x8, 1);
        OLED_PrintASCIIString(0, 32, "CHIP ERASED!",     &afont16x8, 1);
        OLED_ShowFrame();

    #elif BURN_STEP >= 1 && BURN_STEP <= 8
        // ---------------- [模式 1-8: 数据烧录] ----------------
        
        // 1. 计算本次写入的起始地址
        uint32_t write_addr = (BURN_STEP - 1) * PART_SIZE;
        
        // 2. 在 OLED 上显示当前正在干什么
        sprintf(lcd_buf, "Burning Part %d", BURN_STEP);
        OLED_PrintASCIIString(0, 0, lcd_buf, &afont16x8, 1);
        
        sprintf(lcd_buf, "Addr: 0x%X", write_addr); // 显示十六进制地址
        OLED_PrintASCIIString(0, 16, lcd_buf, &afont16x8, 1);
        
        sprintf(lcd_buf, "Size: %d KB", sizeof(CURRENT_ARRAY)/1024);
        OLED_PrintASCIIString(0, 32, lcd_buf, &afont16x8, 1);
        
        OLED_ShowFrame(); // 刷新屏幕显示

        // 3. 执行写入 (阻塞式，直到写完)
        // CURRENT_ARRAY 在 burn_data.h 中被自动映射到 Font_Part_X
        W25Q_Write_NoCheck((uint8_t*)CURRENT_ARRAY, write_addr, sizeof(CURRENT_ARRAY));

        // 4. 写入完成，显示结果
        sprintf(lcd_buf, "Part %d Finished", BURN_STEP);
        OLED_PrintASCIIString(0, 48, lcd_buf, &afont16x8, 1);
        OLED_ShowFrame();

    #else
        #error "[Main Error] Please check BURN_STEP in burn_data.h"
    #endif
}