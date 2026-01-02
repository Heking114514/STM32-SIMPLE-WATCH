#include "game.h"
#include "main.h"
#include "oled.h"
#include "key.h"
#include "menu_core.h"
#include <stdlib.h> // for rand, srand
#include <string.h> // for memset
#include <stdint.h>

// ============================================================================
//   俄罗斯方块 (Tetris) 配置与常量
// ============================================================================

#define T_BLOCK_SIZE  6   // 方块变大为 6x6
#define T_COLS        10  // 10列 (宽度 10*6 = 60px)
#define T_ROWS        18  // 18行 (高度 18*6 = 108px)

// 布局偏移
// 物理X轴方向是游戏的垂直轴。我们将游戏区向右移20像素，左边留给分数
#define T_OFFSET_X    20  // 物理X偏移 (给分数留空)
#define T_OFFSET_Y    2   // 物理Y偏移 (垂直居中: (64-60)/2 = 2)

// 游戏状态
typedef enum {
    GAME_START,         // 初始化
    GAME_SELECT_DIFF,   // [新增] 难度选择
    GAME_PLAY,          // 游戏中
    GAME_OVER           // 游戏结束
} GameState_t;

// 新增光标变量
static uint8_t diff_cursor = 1; // 默认选中 Normal (0:Easy, 1:Normal, 2:Hard)

// 定义三个等级的速度 (ms/tick)
#define SPEED_EASY   800  // 很慢，适合新手
#define SPEED_NORMAL 500  // 正常
#define SPEED_HARD   150  // 极快，挑战手速

static GameState_t t_state = GAME_START;
static uint8_t board[T_ROWS][T_COLS]; // 0:空, 1:有方块
static uint32_t t_score = 0;
static uint32_t t_last_drop_tick = 0;
static uint32_t t_speed = 800; // 初始下落间隔(ms)

// 当前方块属性
static uint8_t cur_x, cur_y; // 方块在网格中的坐标
static uint8_t cur_shape_idx;
static uint8_t cur_rot; // 0-3

// --- 7种方块定义 (4x4 矩阵) ---
// 为了省空间，用索引硬编码形状
// 形状数据 [形状][旋转][4行4列] (简化版：只存坐标偏移太复杂，这里用位图思想)
// 实际上为了简单，我们动态计算旋转。这里定义初始形状。
// 0:I, 1:J, 2:L, 3:O, 4:S, 5:T, 6:Z
static const uint8_t shapes[7][4][2] = {
    {{0,1}, {1,1}, {2,1}, {3,1}}, // I
    {{0,0}, {0,1}, {1,1}, {2,1}}, // J
    {{2,0}, {0,1}, {1,1}, {2,1}}, // L
    {{1,0}, {2,0}, {1,1}, {2,1}}, // O
    {{1,0}, {2,0}, {0,1}, {1,1}}, // S
    {{1,0}, {0,1}, {1,1}, {2,1}}, // T
    {{0,0}, {1,0}, {1,1}, {2,1}}  // Z
};

// ============================================================================
//   内部辅助函数
// ============================================================================

// 获取方块内某个点在旋转后的实际坐标
// rot: 0=0度, 1=90度, 2=180度, 3=270度


// 辅助：绘制竖屏模式下的方块
// grid_x: 0-9 (列)
// grid_y: 0-17 (行)
// type: 0=清除(黑), 1=实心(白), 2=空心框(用于活动方块)



void DrawPixel_Rotated90(int16_t x, int16_t y, uint8_t color) {
    // 逻辑坐标 (x, y) -> 物理坐标 (px, py)
    // 逻辑 X (文字行进方向) -> 物理 Y (从上到下)
    // 逻辑 Y (文字高度方向) -> 物理 X (从左到右)
    
    // 映射关系：
    // Phy X = y + T_OFFSET_X_TEXT (文字基线偏移)
    // Phy Y = x
    
    // 这里我们直接传入绝对物理坐标，由上层函数计算
    OLED_SetPixel(x, y, color ? OLED_COLOR_NORMAL : OLED_COLOR_REVERSED);
}

// 绘制一个旋转 90 度的字符
// x, y: 物理坐标起点
// ch: 字符
// font: 字体
void DrawChar_Rotated(int16_t x, int16_t y, char ch, const ASCIIFont *font) {
    uint8_t w = font->w;
    uint8_t h = font->h;
    
    const uint8_t *data = font->chars + (ch - ' ') * (((h + 7) / 8) * w);
    
    for (uint8_t col = 0; col < w; col++) {
        uint16_t col_data = 0;
        uint8_t bytes_per_col = (h + 7) / 8;
        for (uint8_t b = 0; b < bytes_per_col; b++) {
             col_data |= ((uint16_t)data[col * bytes_per_col + b]) << (b * 8);
        }

        for (uint8_t row = 0; row < h; row++) {
            if (col_data & (1 << row)) {
                // 修改此处实现镜像翻转
                // x = x + (h - 1 - row);
                // y = y + (w - 1 - col);
                OLED_SetPixel(x + row, y + (w - 1 - col), OLED_COLOR_NORMAL);
            }
        }
    }
}

// 绘制旋转字符串
void DrawString_Rotated(int16_t x, int16_t y, char *str, const ASCIIFont *font) {
    while (*str) {
        DrawChar_Rotated(x, y, *str, font);
        // 移动到下一个字符位置
        // 物理 Y 增加 (字宽 + 间隔)
        y -= font->w; 
        str++;
    }
}


void DrawRotatedBlock(int8_t grid_x, int8_t grid_y, uint8_t type) {
    if (grid_x < 0 || grid_x >= T_COLS || grid_y < 0 || grid_y >= T_ROWS) return;

    // 坐标变换核心
    // 物理 X = Offset + 行号 * 块大小 (重力方向)
    uint8_t px = T_OFFSET_X + grid_y * T_BLOCK_SIZE;
    
    // 物理 Y = 63 - Offset - (列号+1)*块大小 (因为物理Y=0是上边，我们要让grid_x=0在视觉左边/物理下边)
    // 让我们简化一点：假设手持设备，按钮在右侧。
    // 物理(0,0)是左上。
    // 我们定义：物理Y越大，代表游戏里的越“左”（假设头向左歪看屏幕）
    // 或者更直观：物理Y的 0~63 对应 游戏列的 9~0
    
    // 方案：grid_x (0..9) 映射到 物理 Y (2..62)
    // grid_x=0 -> 物理Y最大 (底部) ? 不，通常 grid_x=0 是左边。
    // 假设我们竖着拿，USB口朝下（如果USB在窄边）。
    // 物理坐标系：X横向长，Y纵向短。
    // 旋转90度后：
    // Game X (列) = 物理 Y
    // Game Y (行) = 物理 X
    
    // 修正映射：让 grid_x=0 在物理Y的小值端 (视觉左侧，物理上侧)
    // 修正居中：(64 - 10*6)/2 = 2
    uint8_t py = 2 + grid_x * T_BLOCK_SIZE;

    // 绘制
    if (type == 0) { // 清除
        OLED_DrawFilledRectangle(px, py, T_BLOCK_SIZE, T_BLOCK_SIZE, OLED_COLOR_REVERSED);
    } 
    else if (type == 1) { // 实心
        OLED_DrawFilledRectangle(px, py, T_BLOCK_SIZE-1, T_BLOCK_SIZE-1, OLED_COLOR_NORMAL);
    }
    else if (type == 2) { // 空心 (活动方块)
        OLED_DrawRectangle(px, py, T_BLOCK_SIZE, T_BLOCK_SIZE, OLED_COLOR_NORMAL);
        // 中间点个点
        OLED_SetPixel(px + 2, py + 2, OLED_COLOR_NORMAL);
    }
}


void GetRotatedPoint(uint8_t shape_idx, uint8_t rot, uint8_t p_idx, int8_t *x, int8_t *y) {
    int8_t px = shapes[shape_idx][p_idx][0];
    int8_t py = shapes[shape_idx][p_idx][1];
    
    // 旋转算法 (绕中心点 1.5, 1.5 旋转? 简化处理，绕 1,1 旋转)
    // 这里采用简化的SRS偏移或者是硬编码旋转
    // 简单旋转：x' = y, y' = 3 - x (针对4x4)
    // 但我们的定义是基于紧凑坐标的。
    
    for (uint8_t i = 0; i < rot; i++) {
        int8_t tmp = px;
        px = 3 - py; // 顺时针旋转90度
        py = tmp;
        
        // 修正偏移，让方块靠左上
        // 这一步比较繁琐，为了代码短，我们允许旋转后有些许位移
    }
    *x = px;
    *y = py;
}

// 碰撞检测 (返回 1 表示有碰撞/越界)
uint8_t CheckCollision(int8_t grid_x, int8_t grid_y, uint8_t rot) {
    for (uint8_t i = 0; i < 4; i++) {
        int8_t px, py;
        GetRotatedPoint(cur_shape_idx, rot, i, &px, &py);
        int8_t real_x = grid_x + px;
        int8_t real_y = grid_y + py;

        // 1. 忽略空像素 (其实shapes里定义的都是实像素)
        
        // 2. 检查边界
        if (real_x < 0 || real_x >= T_COLS || real_y >= T_ROWS) {
            return 1;
        }

        // 3. 检查有没有碰到已经存在的方块
        // 注意：新生成的方块可能在负y轴（屏幕外），不判碰撞，只判越界
        if (real_y >= 0) {
            if (board[real_y][real_x]) return 1;
        }
    }
    return 0;
}

// 固定方块到棋盘
void LockBlock(void) {
    for (uint8_t i = 0; i < 4; i++) {
        int8_t px, py;
        GetRotatedPoint(cur_shape_idx, cur_rot, i, &px, &py);
        int8_t real_x = cur_x + px;
        int8_t real_y = cur_y + py;
        if (real_y >= 0 && real_x >= 0 && real_x < T_COLS && real_y < T_ROWS) {
            board[real_y][real_x] = 1;
        }
    }
}

// 消除满行
void ClearLines(void) {
    for (int8_t y = T_ROWS - 1; y >= 0; y--) {
        uint8_t full = 1;
        for (uint8_t x = 0; x < T_COLS; x++) {
            if (board[y][x] == 0) {
                full = 0;
                break;
            }
        }
        
        if (full) {
            // 消除这一行，上面的所有行下移
            for (int8_t ky = y; ky > 0; ky--) {
                for (uint8_t kx = 0; kx < T_COLS; kx++) {
                    board[ky][kx] = board[ky-1][kx];
                }
            }
            // 顶行清空
            for (uint8_t kx = 0; kx < T_COLS; kx++) board[0][kx] = 0;
            
            t_score += 100;
            if (t_speed > 200) t_speed -= 20; // 难度增加
            y++; // 重新检查这一行（因为上面的落下来了）
        }
    }
}

// 生成新方块
void SpawnBlock(void) {
    cur_shape_idx = rand() % 7;
    cur_rot = 0;
    cur_x = T_COLS / 2 - 2;
    cur_y = 0; // 从顶部开始
    
    // 生成即碰撞 -> 游戏结束
    if (CheckCollision(cur_x, cur_y, cur_rot)) {
        t_state = GAME_OVER;
    }
}

// 绘制游戏界面
void DrawTetris(void) {
    OLED_NewFrame();
    
    // 1. 绘制左侧分数区 (物理左侧 X=0~20)
    // 分割线
    OLED_DrawLine(18, 0, 18, 63, OLED_COLOR_NORMAL);
    
    // 显示 "Score"
    // 修改坐标：X=2, Y=60 (从底部开始往上写)
    // 字体 afont8x6 宽6，"Score" 5个字占30像素，60-30=30，空间足够
    DrawString_Rotated(2, 58, "Score", &afont8x6);
    
    // 显示分数值
    char buf[10];
    sprintf(buf, "%ld", t_score);
    // 修改坐标：写在 Score 上方，Y=28
    DrawString_Rotated(2, 26, buf, &afont8x6);

    // 2. 绘制棋盘 (保持不变)
    for (uint8_t y = 0; y < T_ROWS; y++) {
        for (uint8_t x = 0; x < T_COLS; x++) {
            if (board[y][x]) {
                DrawRotatedBlock(x, y, 1);
            }
        }
    }

    // 3. 绘制活动方块 (保持不变)
    if (t_state == GAME_PLAY) {
        for (uint8_t i = 0; i < 4; i++) {
            int8_t px, py;
            GetRotatedPoint(cur_shape_idx, cur_rot, i, &px, &py);
            DrawRotatedBlock(cur_x + px, cur_y + py, 2);
        }
    }
    
    // 4. Game Over 提示
    if (t_state == GAME_OVER) {
        // 框位置不变
        OLED_DrawFilledRectangle(40, 10, 40, 44, OLED_COLOR_REVERSED);
        OLED_DrawRectangle(40, 10, 40, 44, OLED_COLOR_NORMAL);
        
        // 修改文字坐标 (从 Y 大值开始)
        // "GAME": Y=50
        // "OVER": Y=50
        DrawString_Rotated(54, 40, "GAME", &afont8x6);
        DrawString_Rotated(64, 40, "OVER", &afont8x6);
    }

    OLED_ShowFrame();
}


// 绘制难度选择菜单 (横屏正常模式)
void DrawDifficultyMenu(void) {
    OLED_NewFrame();

    // 1. 标题
    OLED_PrintASCIIString(25, 0, "Select Level", &afont12x6, OLED_COLOR_NORMAL);
    OLED_DrawLine(0, 12, 128, 12, OLED_COLOR_NORMAL);

    // 2. 选项内容
    const char *levels[] = {"Easy   (Slow)", "Normal (Med)", "Hard   (Fast)"};
    
    // 3. 循环绘制列表
    for (uint8_t i = 0; i < 3; i++) {
        uint8_t y = 16 + i * 16; // 行距16
        
        if (i == diff_cursor) {
            // 选中项：白底黑字
            OLED_DrawFilledRectangle(0, y, 128, 16, OLED_COLOR_NORMAL);
            OLED_PrintASCIIString(10, y + 2, (char*)levels[i], &afont12x6, OLED_COLOR_REVERSED);
        } else {
            // 未选项：黑底白字
            OLED_PrintASCIIString(10, y + 2, (char*)levels[i], &afont12x6, OLED_COLOR_NORMAL);
        }
    }

    OLED_ShowFrame();
}


// ============================================================================
//   游戏主循环 (外部调用)
// ============================================================================
void App_Tetris_Loop(void) {
    static uint8_t inited = 0;

    // 1. 全局初始化
    if (!inited || t_state == GAME_START) {
        srand(HAL_GetTick());
        t_state = GAME_SELECT_DIFF; // 默认进入选择界面
        diff_cursor = 1;            // 默认选中 Normal
        inited = 1;
    }

    // =========================================================
    //   阶段 A: 难度选择 (横屏交互)
    // =========================================================
    if (t_state == GAME_SELECT_DIFF) {
        DrawDifficultyMenu();

        // KEY1 (UP): 上一项
        if (Key_IsSingleClick(KEY3_ID)) {
            if (diff_cursor > 0) diff_cursor--;
            else diff_cursor = 2;
        }
        
        // KEY3 (DOWN): 下一项
        if (Key_IsSingleClick(KEY1_ID)) {
            if (diff_cursor < 2) diff_cursor++;
            else diff_cursor = 0;
        }

        // KEY2 (OK): 确认 -> 进入竖屏游戏
        if (Key_IsSingleClick(KEY2_ID)) {
            // 设置速度
            switch (diff_cursor) {
                case 0: t_speed = SPEED_EASY;   break;
                case 1: t_speed = SPEED_NORMAL; break;
                case 2: t_speed = SPEED_HARD;   break;
            }
            
            // 初始化游戏棋盘
            memset(board, 0, sizeof(board));
            t_score = 0;
            SpawnBlock();
            t_last_drop_tick = HAL_GetTick();
            
            // 切换到游戏状态 (此时用户需要把设备转90度)
            t_state = GAME_PLAY;
        }
    }
    // =========================================================
    //   阶段 B: 游戏中 (竖屏交互)
    // =========================================================
    else if (t_state == GAME_PLAY) {
        // ... (这里的代码完全不用动，保持原来的游戏逻辑) ...
        // 为了完整性，我简写在这里：
        
        // 自动下落逻辑...
        if (HAL_GetTick() - t_last_drop_tick >= t_speed) {
            t_last_drop_tick = HAL_GetTick();
            if (!CheckCollision(cur_x, cur_y + 1, cur_rot)) {
                cur_y++;
            } else {
                LockBlock();
                ClearLines();
                SpawnBlock();
            }
        }

        // 按键逻辑 (注意这里是竖屏逻辑)
        if (Key_IsSingleClick(KEY1_ID)) { // 物理上 -> 逻辑左
            if (!CheckCollision(cur_x - 1, cur_y, cur_rot)) cur_x--;
        }
        if (Key_IsSingleClick(KEY3_ID)) { // 物理下 -> 逻辑右
            if (!CheckCollision(cur_x + 1, cur_y, cur_rot)) cur_x++;
        }
        if (Key_IsSingleClick(KEY2_ID)) { // 旋转
            uint8_t next_rot = (cur_rot + 1) % 4;
            if (!CheckCollision(cur_x, cur_y, next_rot)) cur_rot = next_rot;
            // ... 踢墙逻辑 ...
        }
        
        DrawTetris(); // 绘制竖屏游戏画面
    }
    // ... (GAME_OVER 部分保持不变) ...
    else if (t_state == GAME_OVER) {
         DrawTetris(); 
         if (Key_IsSingleClick(KEY1_ID) || Key_IsSingleClick(KEY2_ID) || Key_IsSingleClick(KEY3_ID)) {
            t_state = GAME_START;
            inited = 0;
            Menu_SwitchToMenu();
        }
    }
}

// ============================================================================
//   恐龙游戏 (Dino) - 占位符
// ============================================================================
// void App_Dino_Loop(void) {
//     OLED_NewFrame();
    
//     // 简单的提示界面
//     OLED_PrintASCIIString(20, 20, "Dino Game", &afont12x6, OLED_COLOR_NORMAL);
//     OLED_PrintASCIIString(20, 40, "Coming Soon", &afont8x6, OLED_COLOR_NORMAL);
    
//     OLED_ShowFrame();

//     // 按任意键退出返回菜单
//     if (Key_IsSingleClick(KEY1_ID) || Key_IsSingleClick(KEY2_ID) || Key_IsSingleClick(KEY3_ID)) {
//         Menu_SwitchToMenu();
//     }
// }