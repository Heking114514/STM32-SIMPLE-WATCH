#include "key.h"

// 记录每个按键的锁存状态 (0:未锁定/已松开, 1:已锁定/正在按)
static uint8_t key_locks[5] = {0, 0, 0, 0, 0};

/**
 * @brief  读取按键物理电平并标准化
 * @note   根据你提供的旧代码逻辑进行适配：
 *         KEY1, KEY2, KEY3 -> 高电平有效 (GPIO_PIN_SET)
 *         KEY4             -> 低电平有效 (GPIO_PIN_RESET)
 * @param  key_id: KEY1_ID ~ KEY4_ID
 * @retval 0: 按下 (统一标准), 1: 松开
 */
uint8_t Key_GetRawState(uint8_t key_id)
{
    GPIO_PinState state;

    switch (key_id)
    {
        case KEY2_ID:
            state = HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin);
            // 旧代码逻辑：READ_KEY1 == 1 是按下
            // 所以如果是 SET(1)，返回 0(代表被按下)
            return (state == GPIO_PIN_SET) ? 0 : 1;

        case KEY3_ID:
            state = HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin);
            // 旧代码逻辑：READ_KEY2 == 1 是按下
            return (state == GPIO_PIN_SET) ? 0 : 1;

        case KEY1_ID:
            state = HAL_GPIO_ReadPin(KEY3_GPIO_Port, KEY3_Pin);
            // 旧代码逻辑：READ_KEY3 == 1 是按下
            return (state == GPIO_PIN_SET) ? 0 : 1;

        case KEY4_ID:
            state = HAL_GPIO_ReadPin(KEY4_GPIO_Port, KEY4_Pin);
            // 旧代码逻辑：READ_KEY4 == 0 是按下
            // 所以如果是 RESET(0)，返回 0(代表被按下)
            return (state == GPIO_PIN_RESET) ? 0 : 1;

        default:
            return 1; // 无效ID视为松开
    }
}

/**
 * @brief  检测单击 (带消抖和锁存)
 * @note   该函数需要在主循环中频繁调用
 * @retval 1: 发生了单击, 0: 无动作
 */
uint8_t Key_IsSingleClick(uint8_t key_id)
{
    if (key_id > 4 || key_id == 0) return 0;

    // 1. 读取当前标准化状态 (0=按下, 1=松开)
    uint8_t current_state = Key_GetRawState(key_id);

    // 2. 状态机逻辑
    if (current_state == 0) // 如果被按下
    {
        if (key_locks[key_id] == 0) // 且之前没锁住 (这是新的一次按下)
        {
            HAL_Delay(10); // 简单的阻塞消抖
            
            // 再次确认
            if (Key_GetRawState(key_id) == 0) 
            {
                key_locks[key_id] = 1; // 上锁，防止连发
                return 1; // 触发单击事件！
            }
        }
    }
    else // 如果松开
    {
        key_locks[key_id] = 0; // 解锁，准备下一次点击
    }

    return 0;
}