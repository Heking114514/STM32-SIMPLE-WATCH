#include "sys_params.h"
#include "w25qxx.h"
#include "oled.h"
#include "mp3_player.h"
#include "app_timer.h"
#include "app_power.h"
#include <string.h> // for memset

// 全局参数实例
SysParams_t g_sys_params;

// --- 内部函数：计算校验和 ---
// 简单的累加校验，防止数据损坏
static uint32_t Calc_Checksum(SysParams_t *p) {
    uint32_t sum = 0;
    uint8_t *ptr = (uint8_t*)p;
    uint32_t len = sizeof(SysParams_t) - sizeof(uint32_t);
    
    for (uint32_t i = 0; i < len; i++) {
        sum += ptr[i];
    }
    return sum;
}

// --- 内部函数：加载默认值 ---
// 当 Flash 为空或数据损坏时调用
static void Load_Defaults(void) {
    g_sys_params.magic = PARAM_MAGIC_NUM;
    
    // 设置你的默认参数
    g_sys_params.volume_mute = 0;      // 默认有声
    g_sys_params.brightness  = 255;    // 默认最亮
    g_sys_params.sleep_time  = 10000;  // 默认10秒息屏
    // 【新增】闹钟默认值 (7:00, 关闭)
    g_sys_params.alarm_h = 7;
    g_sys_params.alarm_m = 0;
    g_sys_params.alarm_on = 0; 
    
    // 计算校验和
    g_sys_params.checksum = Calc_Checksum(&g_sys_params);
}

// --- 内部函数：应用参数到硬件 ---
// 将结构体里的值应用到各个驱动模块
static void Apply_Params(void) {
    // 1. 应用亮度
    OLED_SetBrightness(g_sys_params.brightness);
    
    // 2. 应用静音状态
    // 注意：这里需要把 bool 和 uint8_t 转换一下，虽然 C 里通常通用
    MP3_SetMuteStatus((bool)g_sys_params.volume_mute);
    
    // 3. 应用息屏时间
    Power_SetTimeout(g_sys_params.sleep_time);
    Tools_SetAlarm(g_sys_params.alarm_h, 
                   g_sys_params.alarm_m, 
                   (bool)g_sys_params.alarm_on);
}

// ================= 公共接口 =================

// 开机初始化
void System_Params_Init(void) {
    // 1. 从 Flash 读取整个结构体
    W25Q_Read((uint8_t*)&g_sys_params, PARAM_FLASH_ADDR, sizeof(SysParams_t));
    
    // 2. 验证有效性
    uint32_t cal_sum = Calc_Checksum(&g_sys_params);
    
    if (g_sys_params.magic != PARAM_MAGIC_NUM || g_sys_params.checksum != cal_sum) {
        // 数据无效（第一次开机或损坏），加载默认值并保存
        Load_Defaults();
        System_Params_Save(); // 写入 Flash
    }
    
    // 3. 应用参数
    Apply_Params();
}

// 保存参数到 Flash
void System_Params_Save(void) {
    // 1. 更新校验和
    g_sys_params.checksum = Calc_Checksum(&g_sys_params);
    
    // 2. 擦除扇区 (Flash 写入前必须擦除，最小单位通常是 4KB 扇区)
    // 注意：这会擦除 PARAM_FLASH_ADDR 开始的 4KB 数据
    // 由于我们只存一个结构体，远小于 4KB，所以是安全的
    W25Q_Erase_Sector(PARAM_FLASH_ADDR);
    
    // 3. 写入数据
    W25Q_Write_NoCheck((uint8_t*)&g_sys_params, PARAM_FLASH_ADDR, sizeof(SysParams_t));
    
    // 4. 保存后，建议顺便应用一下（防止调用者只改了参数没应用）
    Apply_Params();
}