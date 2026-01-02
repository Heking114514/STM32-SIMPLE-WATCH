#include "app_dino.h"
#include "main.h"
#include "oled.h"
#include "key.h"
#include "menu_core.h"
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

// 引用 OLED 显存 (oled.c 中定义)
// 注意：需要在 oled.h 或 oled.c 中确保 OLED_GRAM 是可见的，或者在这里声明
extern uint8_t OLED_GRAM[8][128]; 

#ifndef PI
#define PI 3.1415926535f
#endif
static uint8_t jump_height_offset = 0;

// 【新增】Game Over 弹窗光标：0=Restart, 1=Exit
static uint8_t game_over_cursor = 0; 
// ============================================================================
//   资源数据 (图模)
// ============================================================================

const uint8_t Dino_Ground_Data[] = {
	0x0C,0x0C,0x0C,0x0E,0x0E,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x4C,0x6C,0x2C,0x0C,0x0C,
	0x0C,0x0C,0x0C,0x0C,0x0C,0x0E,0x0C,0x0C,0x0E,0x0F,0x3E,0x2E,0x2C,0x0C,0x0C,0x0C,
	0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x2C,0x2C,0x2C,0x0C,0x0C,0x0C,0x0C,0x0E,0x0E,0x0E,
	0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x2C,0x2C,0x2C,0x0C,0x0C,0x1C,0x0C,0x0C,0x0C,
	0x0C,0x0C,0x0C,0x0D,0x0F,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x4C,0x4C,0x4C,0x8C,
	0xDC,0x0C,0x0E,0x0E,0x0E,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,
	0x0C,0x0C,0x2C,0x3C,0x2C,0x1C,0x0E,0x3C,0x3C,0x0C,0x1C,0x0C,0x0C,0x1C,0x0C,0x0C,
	0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x1C,0x0C,0x0E,0x0C,0x1C,0x7C,0x0C,0x0C,0x0C,0x0C,
	0x0C,0x1C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x3C,0x1C,0x3C,0x2C,0x2C,0x2C,0x3F,
	0x0C,0x4C,0x4C,0x4C,0x4C,0x0C,0x6C,0x4C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,
	0x0C,0x0C,0x0C,0x0C,0x0C,0x1C,0x1C,0x0C,0x0C,0x0C,0x0C,0x1C,0x4C,0x4C,0x0C,0x0C,
	0x0E,0x0E,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x2C,0x2F,0x2E,0x2E,
	0x2E,0xFC,0x3C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x2C,0x0C,0x0C,0x0D,0x0D,0x0D,0x0F,
	0x0E,0x0C,0x0C,0x0C,0x1C,0x1C,0x1C,0x1C,0x1C,0x0C,0x0C,0x0C,0x1C,0x7C,0x0C,0x0C,
	0x1C,0x1C,0x3C,0x2C,0x0C,0x2C,0x2C,0x2C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0E,0x0E,0x0E,
	0x0E,0x0E,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x4D,0x4F,0x4E,0x0E,0x0C,0x9C,0x0C,
}; // 256 bytes scrolling ground

// 障碍物数据 (16x18)
const uint8_t Barrier_Data[3][48]={
	//Barrier0
    {
	0x00,0x00,0x00,0x00,0x00,0x00,0xF8,0xFC,0xF8,0x00,0x00,0xF0,0xF0,0x00,0x00,0x00,
	0xC0,0xC0,0xC7,0xCF,0xCC,0x0C,0xFF,0xFF,0xFF,0x03,0xC3,0xC3,0xC1,0xC0,0xC0,0xC0,
	0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00
    },
	//Barrier1
    {
	0x00,0x00,0x00,0x80,0xC0,0x00,0x00,0x00,0x78,0xE0,0xC0,0xFC,0xFC,0x00,0x00,0x00,
	0xC0,0xDC,0x18,0xFF,0xFF,0x06,0x03,0x00,0x00,0x00,0x01,0xFF,0xFF,0x0C,0xCE,0xC6,
	0x00,0x00,0x00,0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x01,0x00,0x00,0x00
    },
	//Barrier2
    {
	0x00,0x00,0x00,0x00,0x00,0xC0,0x00,0xFC,0xF0,0xC0,0x60,0x00,0x00,0x00,0x00,0x00,
	0xC0,0x08,0xFC,0xFE,0x30,0x18,0x01,0xFF,0xFF,0x01,0x00,0x18,0x70,0xFE,0x07,0xC0,
	0x00,0x00,0x01,0x01,0x00,0x00,0x00,0x01,0x01,0x00,0x00,0x00,0x00,0x01,0x00,0x00
    }
};

const uint8_t Cloud_Data[]={
	0x30,0x28,0x28,0x2C,0x26,0x23,0x21,0x21,0x21,0x21,0x23,0x22,0x22,0x2E,0x38,0x30,
};

// 恐龙数据 (16x18)
const uint8_t Dino_Data[3][48]={
	//后腿着地
    {
	0xF0,0xC0,0x00,0x00,0x00,0x80,0x80,0xC0,0xFE,0xFF,0xFB,0xFF,0x7F,0x6F,0x6F,0x0E,
    0xC3,0x07,0x0F,0xFE,0xFF,0x7F,0x1F,0x1F,0x7F,0x7F,0x5F,0x07,0xC1,0xC3,0xC0,0xC0,
    0x00,0x00,0x00,0x01,0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
    },
	//前腿着地
    {
	0xF0,0xC0,0x00,0x00,0x00,0x80,0x80,0xC0,0xFE,0xFF,0xFB,0xFF,0x7F,0x6F,0x6F,0x0E,
    0xC3,0x07,0x0F,0x7E,0x7F,0x5F,0x1F,0x1F,0xFF,0xFF,0x3F,0x07,0xC1,0xC3,0xC0,0xC0,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x01,0x01,0x00,0x00,0x00,0x00,0x00
    },
	//双腿着地 (跳跃)
    {
	0xF0,0xC0,0x00,0x00,0x00,0x80,0x80,0xC0,0xFE,0xFF,0xFB,0xFF,0x7F,0x6F,0x6F,0x0E,
    0x03,0x07,0x0F,0xFE,0xFF,0x7F,0x1F,0x1F,0xFF,0xFF,0x7F,0x07,0x01,0x03,0x00,0x00,
    0x00,0x00,0x00,0x01,0x01,0x01,0x00,0x00,0x01,0x01,0x01,0x00,0x00,0x00,0x00,0x00
    }
};

// 封装成你的 oled.h 中定义的 Image 结构体
const Image img_cloud = {16, 8, Cloud_Data};
const Image img_barrier[3] = {
    {16, 18, Barrier_Data[0]},
    {16, 18, Barrier_Data[1]},
    {16, 18, Barrier_Data[2]}
};
const Image img_dino[3] = {
    {16, 18, Dino_Data[0]},
    {16, 18, Dino_Data[1]},
    {16, 18, Dino_Data[2]}
};

// ============================================================================
//   游戏逻辑变量
// ============================================================================

typedef enum {
    DINO_STATE_INIT,
    DINO_STATE_PLAYING,
    DINO_STATE_GAMEOVER
} DinoState_t;

typedef struct {
    uint8_t minX, minY, maxX, maxY;
} Object_Pos;

static DinoState_t game_state = DINO_STATE_INIT;
static int score = 0;
static uint16_t ground_pos = 0;
static uint8_t barrier_type = 0;
static uint8_t barrier_pos = 0; // 这里的 pos 指的是在屏幕上的相对移动，原逻辑有点绕，我们沿用
static uint8_t cloud_pos = 0;
static uint8_t is_jumping = 0;
static uint16_t jump_tick = 0;
// static uint8_t jump_height_offset = 0;

static Object_Pos pos_dino;
static Object_Pos pos_barrier;

// 难度控制
static uint32_t last_frame_tick = 0;
#define FRAME_INTERVAL 30 // 33ms ~ 30FPS

// ============================================================================
//   内部函数实现
// ============================================================================

static void Dino_InitGame(void) {
    score = 0;
    ground_pos = 0;
    barrier_pos = 0;
    cloud_pos = 0;
    jump_tick = 0;
    is_jumping = 0;
    jump_height_offset = 0;
    barrier_type = 0;
    
    game_over_cursor = 0; // 【新增】重置光标默认在重开上
    
    srand(HAL_GetTick());
}

static void Show_Score(void) {
    char buf[10];
    sprintf(buf, "%05d", score);
    OLED_PrintASCIIString(94, 0, buf, &afont8x6, OLED_COLOR_NORMAL);
}

// 核心：利用显存直接操作来实现地面滚动
static void Show_Ground(void) {
    // 地面数据高度是8 (1页)，位于屏幕最底部 (Page 7)
    // 原始数据长度 256
    
    // 如果 ground_pos < 128，直接复制一段
    // 如果 ground_pos 很大，需要拼一段
    uint8_t *page7 = OLED_GRAM[7]; // 获取显存第7页的指针
    
    // 模拟原代码逻辑：直接写入显存
    if(ground_pos < 128) {
        for(uint8_t i = 0; i < 128; i++) {
            page7[i] |= Dino_Ground_Data[i + ground_pos]; // 使用 |= 防止覆盖分数或其他？不，地面在最底，直接覆盖
            page7[i] = Dino_Ground_Data[i + ground_pos];
        }
    } else {
        // 需要回卷
        // 第一段：从 ground_pos 到 255
        uint8_t part1_len = 256 - ground_pos;
        // 只有当 part1_len < 128 时才需要回卷，但 ground_pos >= 128，所以 part1_len 最大128
        for(uint8_t i = 0; i < part1_len; i++) {
            page7[i] = Dino_Ground_Data[i + ground_pos];
        }
        // 第二段：从 0 开始补齐
        for(uint8_t i = part1_len; i < 128; i++) {
            page7[i] = Dino_Ground_Data[i - part1_len];
        }
    }
}

static void Show_Barrier(void) {
    // Barrier_Pos 在原代码中是一个递增的值，当 >= 143 时重置
    // 绘制坐标 X = 127 - Barrier_Pos
    
    int16_t draw_x = 127 - barrier_pos;
    
    if (barrier_pos >= 143) {
        barrier_type = rand() % 3;
        barrier_pos = 0; // 重置障碍物位置
        draw_x = 127;
    }
    
    // Y 坐标 44
    // 注意：Barrier 是 16x18，跨越了 Page 5,6,7
    OLED_DrawImage((uint8_t)draw_x, 44, &img_barrier[barrier_type], OLED_COLOR_NORMAL);
    
    // 更新碰撞箱
    pos_barrier.minX = (draw_x < 0) ? 0 : draw_x;
    pos_barrier.maxX = draw_x + 16;
    pos_barrier.minY = 44;
    pos_barrier.maxY = 62;
    
    // 如果障碍物移出屏幕左边，不要让 maxX 太大干扰判断(虽然原逻辑没处理)
    if (draw_x < -16) { 
        pos_barrier.maxX = 0; 
        pos_barrier.minX = 0;
    }
}

static void Show_Cloud(void) {
    int16_t draw_x = 127 - cloud_pos;
    OLED_DrawImage((uint8_t)draw_x, 9, &img_cloud, OLED_COLOR_NORMAL);
}

static void Show_Dino(void) {
    // 跳跃计算
    if (is_jumping) {
        // 原逻辑：Jump_Pos = 28 * sin(pi * jump_t / 1000)
        // jump_t 每次 + 20 (因为原逻辑是一次Tick加1，这里我们按时间比例)
        // 让我们简化一下：
        // jump_t 范围 0 - 1000 (代表一个完整跳跃周期)
        
        float rad = (float)(PI * jump_tick / 1000.0f);
        jump_height_offset = (uint8_t)(28 * sinf(rad));
        
        // 绘制跳跃图
        OLED_DrawImage(0, 44 - jump_height_offset, &img_dino[2], OLED_COLOR_NORMAL);
    } else {
        // 奔跑动画 (根据 cloud_pos 或 帧数 切换腿)
        // 简单用 cloud_pos % 4 来切换频率
        if ((cloud_pos / 4) % 2 == 0) {
            OLED_DrawImage(0, 44, &img_dino[0], OLED_COLOR_NORMAL);
        } else {
            OLED_DrawImage(0, 44, &img_dino[1], OLED_COLOR_NORMAL);
        }
    }
    
    // 更新恐龙碰撞箱
    pos_dino.minX = 4; // 恐龙虽然在x=0绘制，但有点空白，稍微缩一下判定
    pos_dino.maxX = 12;
    pos_dino.minY = 44 - jump_height_offset + 2; // 稍微缩一下头部
    pos_dino.maxY = 62 - jump_height_offset;
}

static uint8_t Check_Collision(void) {
    // AABB 碰撞检测
    if ((pos_dino.maxX > pos_barrier.minX) && 
        (pos_dino.minX < pos_barrier.maxX) && 
        (pos_dino.maxY > pos_barrier.minY) && 
        (pos_dino.minY < pos_barrier.maxY)) 
    {
        return 1; // 撞了
    }
    return 0;
}

// 物理和逻辑更新 (替代原本的 Dino_Tick)
static void Update_Physics(void) {
    // 1. 分数增加 (每5帧加1分)
    static uint8_t score_div = 0;
    if (++score_div > 5) {
        score++;
        score_div = 0;
    }
    
    // 2. 地面和障碍物移动 (速度快)
    // 每次移动 2-3 像素，增加速度感
    uint8_t speed = 3 + (score / 200); // 随着分数增加速度
    if (speed > 6) speed = 6;

    ground_pos += speed;
    barrier_pos += speed;
    
    if (ground_pos >= 256) ground_pos -= 256;
    // Barrier_pos 的重置逻辑在 Show_Barrier 里处理，这里只管加
    
    // 3. 云朵移动 (速度慢)
    cloud_pos += 1;
    if (cloud_pos >= 200) cloud_pos = 0;
    
    // 4. 跳跃逻辑
    if (is_jumping) {
        // 每次增加的时间步长决定跳跃速度
        // 1000ms 周期，30ms 一帧，约 33 帧完成
        jump_tick += 35; 
        if (jump_tick >= 1000) {
            jump_tick = 0;
            is_jumping = 0;
            jump_height_offset = 0;
        }
    }
}

// ============================================================================
//   主循环 (Public)
// ============================================================================

void App_Dino_Loop(void) {
    
    // 1. 初始化
    if (game_state == DINO_STATE_INIT) {
        Dino_InitGame();
        game_state = DINO_STATE_PLAYING;
    }

    // 2. 帧率控制 (约30FPS)
    // 所有的逻辑和绘图都必须在这个 if 里面执行，保证刷新率稳定，消除闪烁
    if (HAL_GetTick() - last_frame_tick < FRAME_INTERVAL) {
        return;
    }
    last_frame_tick = HAL_GetTick();

    // ==========================================
    //           游戏进行中 (PLAYING)
    // ==========================================
    if (game_state == DINO_STATE_PLAYING) {
        
        // --- A. 输入处理 ---
        // 任何按键跳跃
        if ((Key_IsSingleClick(KEY1_ID) || Key_IsSingleClick(KEY2_ID) || Key_IsSingleClick(KEY3_ID)) && !is_jumping) {
            is_jumping = 1;
            jump_tick = 0;
        }
        // 游戏中按 KEY4 暂停/退出? 这里直接作为退出
        if (Key_IsSingleClick(KEY4_ID)) {
            game_state = DINO_STATE_GAMEOVER; // 或者直接退出，看你需求
            // 如果想直接退出：
            // game_state = DINO_STATE_INIT;
            // Menu_SwitchToMenu();
            // return;
        }

        // --- B. 物理计算 ---
        Update_Physics();
        if (Check_Collision()) {
            game_state = DINO_STATE_GAMEOVER;
            game_over_cursor = 0; // 默认选中重开
        }

        // --- C. 画面渲染 ---
        OLED_NewFrame();
        Show_Ground();
        Show_Cloud();
        Show_Barrier();
        Show_Dino();
        Show_Score();
        OLED_ShowFrame();
    }
    
    // ==========================================
    //           游戏结束 (GAMEOVER) - 弹窗处理
    // ==========================================
    else if (game_state == DINO_STATE_GAMEOVER) {
        
        // --- A. 输入处理 (弹窗交互) ---
        
        // KEY1 (左/上): 选择 "Restart"
        if (Key_IsSingleClick(KEY1_ID)) {
            if (game_over_cursor > 0) game_over_cursor--;
            else game_over_cursor = 1; // 循环
        }
        
        // KEY3 (右/下): 选择 "Exit"
        if (Key_IsSingleClick(KEY3_ID)) {
            if (game_over_cursor < 1) game_over_cursor++;
            else game_over_cursor = 0; // 循环
        }

        // KEY2 (OK): 执行选择
        if (Key_IsSingleClick(KEY2_ID)) {
            if (game_over_cursor == 0) {
                // 选中了 Restart
                Dino_InitGame();
                game_state = DINO_STATE_PLAYING;
            } else {
                // 选中了 Exit
                game_state = DINO_STATE_INIT; // 重置状态
                Menu_SwitchToMenu();          // 返回主菜单
                return;
            }
        }

        // --- B. 画面渲染 ---
        OLED_NewFrame();
        
        // 1. 绘制背景 (最后一帧的游戏画面，让它定格)
        Show_Ground();
        Show_Cloud();
        Show_Barrier(); 
        Show_Dino();
        Show_Score();

        // 2. 绘制半透明蒙版效果 (可选：用稀疏点阵覆盖，这里简单画个框)
        // 绘制弹窗边框 (居中: W=100, H=40 -> X=14, Y=12)
        OLED_DrawFilledRectangle(14, 12, 100, 40, OLED_COLOR_REVERSED); // 白底
        OLED_DrawRectangle(14, 12, 100, 40, OLED_COLOR_NORMAL);         // 黑边框
        
        // 3. 标题
        OLED_PrintASCIIString(38, 16, "GAME OVER", &afont12x6, OLED_COLOR_NORMAL);

        // 4. 绘制选项
        // Option 1: Restart (左)
        if (game_over_cursor == 0) {
            // 选中状态：黑底白字 (画个黑实心框，字用反色)
            OLED_DrawFilledRectangle(18, 32, 44, 12, OLED_COLOR_NORMAL);
            OLED_PrintASCIIString(20, 34, "Restart", &afont8x6, OLED_COLOR_REVERSED);
        } else {
            // 未选中：白底黑字
            OLED_PrintASCIIString(20, 34, "Restart", &afont8x6, OLED_COLOR_NORMAL);
        }

        // Option 2: Exit (右)
        if (game_over_cursor == 1) {
            // 选中状态
            OLED_DrawFilledRectangle(76, 32, 28, 12, OLED_COLOR_NORMAL);
            OLED_PrintASCIIString(78, 34, "Exit", &afont8x6, OLED_COLOR_REVERSED);
        } else {
            // 未选中
            OLED_PrintASCIIString(78, 34, "Exit", &afont8x6, OLED_COLOR_NORMAL);
        }
        
        OLED_ShowFrame();
    }
}