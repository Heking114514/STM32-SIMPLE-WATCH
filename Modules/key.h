#ifndef __KEY_H
#define __KEY_H

#include "main.h" // 引入main.h以获取引脚定义

// 定义按键ID，供上层应用（Menu/Game）调用
// 对应 main.h 中的引脚定义
#define KEY1_ID  1  // 对应 KEY1_Pin (通常是 UP / 左)
#define KEY2_ID  2  // 对应 KEY2_Pin (通常是 OK / 确认)
#define KEY3_ID  3  // 对应 KEY3_Pin (通常是 DOWN / 右)
#define KEY4_ID  4  // 对应 KEY4_Pin (通常是 Back / 返回)

// 函数声明
// 检测指定按键是否被单击（按下瞬间返回1，长按不重复返回）
uint8_t Key_IsSingleClick(uint8_t key_id);

// 检测指定按键的实时电平状态 (0:按下, 1:松开)
uint8_t Key_GetRawState(uint8_t key_id);

#endif