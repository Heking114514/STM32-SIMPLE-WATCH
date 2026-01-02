#include "app_gradienter.h"
#include "main.h"
#include "oled.h"
#include "key.h"
#include "menu_core.h"
#include "mpu6050.h"
#include <math.h> // 需要用到 sqrt

// --- 参数配置 ---
#define CENTER_X        64  // 屏幕中心 X
#define CENTER_Y        32  // 屏幕中心 Y
#define BIG_RADIUS      28  // 外圆半径 (屏幕高度32，留点边距)
#define BUBBLE_RADIUS   5   // 气泡(小圆)半径
#define MAX_MOVE_DIST   (BIG_RADIUS - BUBBLE_RADIUS - 2) // 气泡最大移动距离(留2px缝隙)

// 灵敏度系数：值越小，气泡移动越灵敏 (1.0表示 1度对应1像素)
// 调整这个值来适配手感
#define SENSITIVITY     1.0f 

void App_Gradienter_Loop(void)
{
    // 1. 处理退出逻辑
    if (Key_IsSingleClick(KEY1_ID) || Key_IsSingleClick(KEY2_ID) || Key_IsSingleClick(KEY3_ID))  // 按确认键退出
    {
        Menu_SwitchToMenu();
        return;
    }

    // 2. 获取姿态数据 (使用你现有的卡尔曼滤波数据)
    const MPU6050_t* mpu = MPU6050_GetDataPtr();
    
    // 3. 计算偏移量
    // 根据实际安装方向，可能需要修改正负号
    // 假设：Roll对应X轴偏移，Pitch对应Y轴偏移
    float offset_x = -(mpu->KalmanAngleX * SENSITIVITY); 
    float offset_y = (mpu->KalmanAngleY * SENSITIVITY);

    // 4. 边界限制算法 (防止气泡跑出圆圈)
    // 计算当前距离中心的距离
    float dist = sqrtf(offset_x * offset_x + offset_y * offset_y);
    
    // 如果距离超过了允许的最大半径，进行等比例缩放
    if (dist > MAX_MOVE_DIST)
    {
        float scale = MAX_MOVE_DIST / dist;
        offset_x *= scale;
        offset_y *= scale;
    }

    // 5. 计算最终屏幕坐标
    uint8_t bubble_x = (uint8_t)(CENTER_X + offset_x);
    uint8_t bubble_y = (uint8_t)(CENTER_Y + offset_y);

    // 6. 绘制界面
    OLED_NewFrame();

    // A. 画外圆 (空心)
    OLED_DrawCircle(CENTER_X, CENTER_Y, BIG_RADIUS, OLED_COLOR_NORMAL);
    
    // B. 画十字准星 (中心标记)
    // 横线
    OLED_DrawLine(CENTER_X - 4, CENTER_Y, CENTER_X + 4, CENTER_Y, OLED_COLOR_NORMAL);
    // 竖线
    OLED_DrawLine(CENTER_X, CENTER_Y - 4, CENTER_X, CENTER_Y + 4, OLED_COLOR_NORMAL);

    // C. 画气泡 (实心)
    // 这里的坐标是气泡圆心
    OLED_DrawFilledCircle(bubble_x, bubble_y, BUBBLE_RADIUS, OLED_COLOR_NORMAL);

    // D. (可选) 在角落显示数值
    // 如果想让界面极简，可以把下面这几行注释掉
    /*
    char buf[10];
    sprintf(buf, "R:%.0f", mpu->KalmanAngleX);
    OLED_PrintASCIIString(0, 0, buf, &afont8x6, OLED_COLOR_NORMAL);
    sprintf(buf, "P:%.0f", mpu->KalmanAngleY);
    OLED_PrintASCIIString(0, 56, buf, &afont8x6, OLED_COLOR_NORMAL);
    */

    OLED_ShowFrame();
}