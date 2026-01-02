#ifndef __BURN_DATA_H__
#define __BURN_DATA_H__

// ================== 烧录控制核心 ==================

/**
 * @brief 烧录步骤选择宏
 * 
 * 修改此数值来决定本次编译烧录的内容：
 * 
 * [0]   : 执行全片擦除 (Erase Chip) 
 *         -> 注意：这是第一次使用前必须做的！耗时约 20-40秒。
 * 
 * [1-8] : 烧录对应的字库分片 (Part 1 ~ Part 8)
 *         -> 每次修改后，点击编译，然后下载到单片机。
 */
#define BURN_STEP 7
//#define each_size 261696
// ================== 自动包含逻辑 ==================
// 下面的代码会自动根据上面的 BURN_STEP 选择要包含的文件
// 并将具体的数组名统一映射为 CURRENT_ARRAY
// ================================================

#if BURN_STEP == 1
    #include "font_part_1.h"       // 引入第1部分
    #define CURRENT_ARRAY Font_Part_1

#elif BURN_STEP == 2
    #include "font_part_2.h"       // 引入第2部分
    #define CURRENT_ARRAY Font_Part_2

#elif BURN_STEP == 3
    #include "font_part_3.h"       // 引入第3部分
    #define CURRENT_ARRAY Font_Part_3

#elif BURN_STEP == 4
    #include "font_part_4.h"       // 引入第4部分
    #define CURRENT_ARRAY Font_Part_4

#elif BURN_STEP == 5
    #include "font_part_5.h"       // 引入第5部分
    #define CURRENT_ARRAY Font_Part_5

#elif BURN_STEP == 6
    #include "font_part_6.h"       // 引入第6部分
    #define CURRENT_ARRAY Font_Part_6

#elif BURN_STEP == 7
    #include "font_part_7.h"       // 引入第7部分
    #define CURRENT_ARRAY Font_Part_7

#elif BURN_STEP == 0
    // 擦除模式下不需要包含大数组，防止编译过慢
    #define CURRENT_ARRAY  0 

#else
    #error "[burn_data.h] 配置错误: BURN_STEP 必须是 0 到 8 之间的整数!"
#endif

#endif // __BURN_DATA_H__