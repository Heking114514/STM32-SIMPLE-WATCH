#ifndef __SYS_PARAMS_H
#define __SYS_PARAMS_H

#include <stdint.h>
#include <stdbool.h>

// --- 配置区域 ---
// W25Q128 的最后一个扇区地址 (16MB - 4KB)
#define PARAM_FLASH_ADDR   0x00FFF000 
#define PARAM_MAGIC_NUM    0x5A5A0002 

typedef struct {
    uint32_t magic;        // 魔术字 (头部校验)
    uint8_t  volume_mute;  // 0:正常, 1:静音
    uint8_t  brightness;   // 亮度 (0-255)
    uint32_t sleep_time;   // 息屏时间 (ms)
    
    uint8_t  alarm_h;      // 闹钟时
    uint8_t  alarm_m;      // 闹钟分
    uint8_t  alarm_on;     // 闹钟开关 (0:关, 1:开)

    // [尾部校验]
    uint32_t checksum;     // 校验和
} SysParams_t;

// --- 全局实例声明 ---
extern SysParams_t g_sys_params;

// --- 接口函数 ---
void System_Params_Init(void); 
void System_Params_Save(void); 

#endif