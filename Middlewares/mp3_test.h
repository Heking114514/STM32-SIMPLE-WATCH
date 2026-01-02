#ifndef __MP3_TEST_H
#define __MP3_TEST_H

#include "main.h"

/**
 * @brief  初始化MP3播放器应用
 * @note   此函数会初始化MP3模块，并设置一个初始音量。
 *         应在 main 函数的外设初始化之后调用。
 * @param  None
 * @retval None
 */
void MP3_App_Init(void);


/**
 * @brief  MP3播放器应用的循环处理任务
 * @note   此函数内部会处理按键扫描、MP3状态监听和OLED显示。
 *         必须在 main 函数的 while(1) 循环中被持续调用。
 * @param  None
 * @retval None
 */
void MP3_App_Loop(void);


#endif /* __MP3_APP_H */