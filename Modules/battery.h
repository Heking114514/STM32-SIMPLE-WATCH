#ifndef __BATTERY_H
#define __BATTERY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h" // 包含你的芯片对应的HAL库头文件

/**
 * @brief  初始化电池检测模块
 * @note   此函数会拉低 BAT_ADC_EN (PB12) 引脚，导通ADC检测电路。
 *         应在 main 函数的 GPIO 初始化之后调用。
 * @param  None
 * @retval None
 */
void Battery_Init(void);

/**
 * @brief  获取电池电压对应的ADC原始平均值
 * @note   内部会进行3000次采样并取平均值，有一定耗时。
 * @param  None
 * @retval uint16_t ADC平均值 (范围: 0-4095)
 */
uint16_t Battery_Get_ADC_Value(void);

/**
 * @brief  获取计算后的电池电压值
 * @param  None
 * @retval float 电池电压值 (单位: V)
 */
float Battery_Get_Voltage(void);

/**
 * @brief  获取计算后的电池电量百分比
 * @note   使用 (ADC_Value - 3276) * 100 / 819 的逻辑进行计算。
 * @param  None
 * @retval int 电池电量百分比 (范围: 0-100)
 */
int Battery_Get_Percentage(void);


#ifdef __cplusplus
}
#endif

#endif /* __BATTERY_H */