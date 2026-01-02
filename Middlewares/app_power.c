#include "app_power.h"
#include "main.h"
#include "oled.h"
#include "key.h"
#include "mpu6050.h"
#include <math.h> // 需要用到 fabs

// ================= 配置参数 =================

// 运动唤醒阈值 (角速度之和)
// MPU6050输出的 Gx, Gy, Gz 单位通常是 度/秒(°/s) 或 弧度/秒
// 建议值：150.0 ~ 300.0
// 值越小越灵敏（轻轻动一下就亮），值越大需要甩得越快
#define WAKE_MOTION_THRESHOLD  500.0f  

// 连续检测计数器阈值 (防抖)
// 只有连续 N 次检测到剧烈运动才唤醒，防止偶尔震动误触
#define WAKE_COUNT_THRESHOLD   2

// ===========================================

// --- 内部状态 ---
typedef enum {
    SYS_ACTIVE,
    SYS_SLEEP
} SystemState_t;

static uint32_t g_sleep_timeout = 10000; 
static SystemState_t current_state = SYS_ACTIVE;
static uint32_t last_activity_tick = 0;

// 外部引用 MPU 数据获取接口
extern const MPU6050_t* MPU6050_GetDataPtr(void);

// --- 内部辅助函数 ---

void Power_SetTimeout(uint32_t ms) {
    g_sleep_timeout = ms;
    Power_ResetTimer(); 
}

uint32_t Power_GetTimeout(void) {
    return g_sleep_timeout;
}

static void Enter_Sleep(void) {
    if (current_state == SYS_SLEEP) return;
    
    OLED_DisPlay_Off(); // 关屏
    // 这里可以添加关闭 LED、降低主频等逻辑
    current_state = SYS_SLEEP;
}

static void Exit_Sleep(void) {
    if (current_state == SYS_ACTIVE) return;
    
    OLED_DisPlay_On();  // 开屏
    Power_ResetTimer(); // 重置熄屏倒计时
    current_state = SYS_ACTIVE;
}

// 检查是否满足“剧烈运动”唤醒条件
// 返回 true 表示检测到了抬手/甩手动作
static bool Check_Raise_Wakeup(void) {
    static uint8_t motion_cnt = 0; // 动作持续计数器
    const MPU6050_t* mpu = MPU6050_GetDataPtr();
    
    // 计算当前的运动强度
    // 原理：静止时角速度(G)接近0，运动越快G越大
    // 使用绝对值之和来评估整体运动量
    double total_motion = fabs(mpu->Gx) + fabs(mpu->Gy) + fabs(mpu->Gz);
    
    // 调试用：如果不确定阈值，可以在这里打印 total_motion 到串口
    // printf("Motion: %f\n", total_motion);

    if (total_motion > WAKE_MOTION_THRESHOLD) {
        motion_cnt++;
    } else {
        motion_cnt = 0; // 如果动作中断，计数清零
    }

    // 只有连续检测到几次高强度运动，才认为是有效抬手
    if (motion_cnt >= WAKE_COUNT_THRESHOLD) {
        motion_cnt = 0; // 重置
        return true;
    }
    
    return false;
}

// --- 公共接口 ---

void Power_Init(void) {
    current_state = SYS_ACTIVE;
    last_activity_tick = HAL_GetTick();
}

void Power_ResetTimer(void) {
    last_activity_tick = HAL_GetTick();
}

bool Power_Update(void) {
    uint32_t now = HAL_GetTick();

    // 1. 按键物理状态检测 (低电平有效还是高电平有效请根据实际硬件确认)
    // 假设 Key_GetRawState 返回 0 表示按下
    bool key_pressed = false;
    if (Key_GetRawState(KEY1_ID) == 0 || 
        Key_GetRawState(KEY2_ID) == 0 || 
        Key_GetRawState(KEY3_ID) == 0 || 
        Key_GetRawState(KEY4_ID) == 0) {
        key_pressed = true;
    }

    // ================= 状态机逻辑 =================
    
    if (current_state == SYS_ACTIVE) {
        // --- [活跃模式] ---
        
        // A. 有按键操作，重置熄屏计时
        if (key_pressed) {
            Power_ResetTimer();
        }
        
        // B. 检查超时 -> 进入休眠
        // (g_sleep_timeout 为 0 时代表永不休眠)
        if (g_sleep_timeout > 0 && (now - last_activity_tick > g_sleep_timeout)) {
            Enter_Sleep();
            return false; // 停止 UI 刷新
        }
        
        return true; // 继续运行 UI
    } 
    else {
        // --- [休眠模式] ---
        
        // A. 检测按键唤醒
        if (key_pressed) {
            // 消耗掉这次按键事件，防止点亮瞬间误触菜单
            Key_IsSingleClick(KEY1_ID);
            Key_IsSingleClick(KEY2_ID);
            Key_IsSingleClick(KEY3_ID);
            Key_IsSingleClick(KEY4_ID);
            
            Exit_Sleep();
            return true; 
        }
        
        // B. 检测抬手/动作唤醒 (基于角速度变化)
        // 只有当你手腕转动、抬起（有速度）时才会触发
        // 静止平放时，角速度为0，不会触发，从而解决了闪烁问题
        if (Check_Raise_Wakeup()) {
            Exit_Sleep();
            return true;
        }
        
        // 休眠中降低检测频率
        HAL_Delay(50); 
        return false;
    }
}