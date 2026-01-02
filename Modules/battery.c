#include "battery.h"

// 假设你的CubeMX生成的ADC句柄名为 hadc1
extern ADC_HandleTypeDef hadc1;

// --- 配置宏定义 ---
#define BATTERY_ADC_SAMPLES       100      // ADC采样次数
#define ADC_VREF                  3.3f     // ADC参考电压
#define ADC_MAX_VALUE             4095.0f  // 12位ADC的最大值


// ===================================================================================
// !!! 最终校准参数 !!!
// 根据你的实际测量值进行配置
#define BATTERY_ADC_MIN           2000     // 0%电量 (空电) 对应的ADC值
#define BATTERY_ADC_MAX           2500     // 100%电量 (满电) 对应的ADC值
// ===================================================================================


// --- BAT_ADC_EN 引脚定义 ---
#ifndef BAT_ADC_EN_GPIO_Port
#define BAT_ADC_EN_GPIO_Port      GPIOB
#endif
#ifndef BAT_ADC_EN_Pin
#define BAT_ADC_EN_Pin            GPIO_PIN_12
#endif


/**
 * @brief  初始化电池检测模块
 */
void Battery_Init(void)
{
    HAL_GPIO_WritePin(BAT_ADC_EN_GPIO_Port, BAT_ADC_EN_Pin, GPIO_PIN_RESET);
}

/**
 * @brief  获取电池电压对应的ADC原始平均值
 */
uint16_t Battery_Get_ADC_Value(void)
{
    uint32_t sum = 0;

    for (int i = 0; i < BATTERY_ADC_SAMPLES; i++)
    {
        HAL_ADC_Start(&hadc1);
        HAL_ADC_PollForConversion(&hadc1, 100);
        sum += HAL_ADC_GetValue(&hadc1);
        HAL_ADC_Stop(&hadc1);
    }
    return (uint16_t)(sum / BATTERY_ADC_SAMPLES);
}

/**
 * @brief  获取计算后的电池电压值 (注意: 此电压为PA0引脚电压，非真实电池电压)
 */
float Battery_Get_Voltage(void)
{
    uint16_t adc_value = Battery_Get_ADC_Value();
    return (float)adc_value / ADC_MAX_VALUE * ADC_VREF;
}

/**
 * @brief  获取计算后的电池电量百分比 (使用最终校准值)
 */
int Battery_Get_Percentage(void)
{
    uint16_t adc_value = Battery_Get_ADC_Value();
    long percentage;

    // 边界处理1：如果当前值低于设定的最小值 (2000)，则电量为 0%
    if (adc_value < BATTERY_ADC_MIN)
    {
        return 0;
    }

    // 边界处理2：如果当前值高于设定的最大值 (2500)，则电量为 100%
    // 这个逻辑完美地处理了你说的 "冲到2510也要算100%" 的情况
    if (adc_value >= BATTERY_ADC_MAX)
    {
        return 100;
    }
    
    // 核心计算公式：在 [2000, 2500] 区间内进行线性映射
    // (当前值 - 最小值) * 100 / (最大值 - 最小值)
    percentage = (long)(adc_value - BATTERY_ADC_MIN) * 100 / (BATTERY_ADC_MAX - BATTERY_ADC_MIN);

    return (int)percentage;
}