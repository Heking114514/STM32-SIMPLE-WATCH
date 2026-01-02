#include "app_setting.h"
#include "clock.h"
#include "oled.h"
#include "key.h"
#include "menu_core.h" // 需要调用 Menu_SwitchToMenu() 退出
#include <stdio.h>
#include "app_power.h" // 引入电源接口
#include "adc.h" // 需要引用 ADC 句柄
#include "sys_params.h" // 【新增】


// --- 内部状态变量 ---
// 使用 static 变量保存编辑时的临时数据
static uint8_t edit_idx = 0; // 当前光标位置 (0, 1, 2)
static uint8_t val[3];       // 暂存 [年,月,日] 或 [时,分,秒]
static uint8_t is_inited = 0;

// 辅助：重置状态
static void ResetState(void) {
    edit_idx = 0;
    is_inited = 0;
}

/*
读取核心温度
*/
extern ADC_HandleTypeDef hadc1;

// ============================================================================
//   内部辅助：读取 MCU 核心温度
// ============================================================================
// STM32F103 内部温度计算公式： T = {(V25 - Vsense) / Avg_Slope} + 25
// V25 = 1.43V, Avg_Slope = 4.3mV/C
static float Get_MCU_Temperature(void) {
    ADC_ChannelConfTypeDef sConfig = {0};
    uint32_t adc_val = 0;
    
    // 1. 配置 ADC 通道为 内部温度传感器 (Channel 16)
    sConfig.Channel = ADC_CHANNEL_TEMPSENSOR;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    // 温度传感器建议采样时间 > 17.1us。
    // 12MHz ADC时钟下，239.5周期约20us，足够。
    sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5; 
    
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
        return 0.0f;
    }
    
    // 2. 启动 ADC 并读取
    // 注意：F103 需要置位 TSVREFE (在 HAL_ADC_Init 中通常通过 Init 结构体处理，
    // 但如果 CubeMX 没开 TempSensor，可能需要手动开启，这里假设 HAL 库能处理或默认已开启)
    // 也可以手动：ADC1->CR2 |= ADC_CR2_TSVREFE; 
    
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 100);
    adc_val = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);
    
    // 3. 恢复原来的电池电压采集配置 (Channel 0)
    // 这一步很重要，否则会影响 Battery_Get_Percentage
    sConfig.Channel = ADC_CHANNEL_0;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
    
    // 4. 计算电压 (参考电压 3.3V)
    float voltage = (float)adc_val * 3.3f / 4095.0f;
    
    // 5. 计算温度
    float temp = ((1.43f - voltage) / 0.0043f) + 25.0f;
    return temp;
}


// ============================================================================
//   应用 1: 日期设置 (Date Setting)
// ============================================================================
void App_Set_Date_Loop(void) {
    // 1. 初始化：读取当前日期
    if (!is_inited) {
        Clock_GetDateValues(&val[0], &val[1], &val[2]); // Y, M, D
        edit_idx = 0;
        is_inited = 1;
    }

    // 2. 绘制 UI
    OLED_NewFrame();
    OLED_PrintASCIIString(0, 0, "Set Date", &afont12x6, OLED_COLOR_NORMAL);

    char buf[20];
    // 显示格式: 20YY - MM - DD
    sprintf(buf, "20%02d-%02d-%02d", val[0], val[1], val[2]);
    OLED_PrintASCIIString(16, 24, buf, &afont16x8, OLED_COLOR_NORMAL);

    // 绘制光标 (下划线)
    // 根据 edit_idx 计算下划线位置。字符宽8像素。
    // "20"占2个字符。第0项(年)从 buf[2]开始。
    // 粗略计算：X起 = 16 + (2 + idx*3)*8
    uint8_t line_x = 16 + (2 + edit_idx * 3) * 8;
    OLED_DrawLine(line_x, 42, line_x + 16, 42, OLED_COLOR_NORMAL);

    OLED_ShowFrame();

    // 3. 处理按键
    if (Key_IsSingleClick(KEY1_ID)) { // UP: 加
        if (edit_idx == 0) val[0] = (val[0] + 1) % 100; // Year 0-99
        if (edit_idx == 1) val[1] = (val[1] % 12) + 1;  // Month 1-12
        if (edit_idx == 2) val[2] = (val[2] % 31) + 1;  // Day 1-31 (简单处理)
    }
    if (Key_IsSingleClick(KEY3_ID)) { // DOWN: 减
        if (edit_idx == 0) val[0] = (val[0] == 0) ? 99 : val[0] - 1;
        if (edit_idx == 1) val[1] = (val[1] == 1) ? 12 : val[1] - 1;
        if (edit_idx == 2) val[2] = (val[2] == 1) ? 31 : val[2] - 1;
    }
    if (Key_IsSingleClick(KEY2_ID)) { // OK: 下一项 或 保存
        edit_idx++;
        if (edit_idx > 2) {
            // 保存并退出
            Clock_SetDate(val[0], val[1], val[2]);
            ResetState(); // 清除状态
            Menu_SwitchToMenu(); // 返回上一级菜单
        }
    }
}

// ============================================================================
//   应用 2: 时间设置 (Time Setting)
// ============================================================================
void App_Set_Time_Loop(void) {
    if (!is_inited) {
        Clock_GetTimeValues(&val[0], &val[1], &val[2]); // H, M, S
        edit_idx = 0;
        is_inited = 1;
    }

    OLED_NewFrame();
    OLED_PrintASCIIString(0, 0, "Set Time (24H)", &afont12x6, OLED_COLOR_NORMAL);

    char buf[20];
    sprintf(buf, "%02d:%02d:%02d", val[0], val[1], val[2]);
    // 居中显示: 128 - 8*8 = 64, start=32
    OLED_PrintASCIIString(32, 24, buf, &afont16x8, OLED_COLOR_NORMAL);

    // 下划线
    // idx=0 -> 00:, idx=1 -> :00:, idx=2 -> :00
    // 间隔是3个字符(24像素)
    uint8_t line_x = 32 + edit_idx * 24;
    OLED_DrawLine(line_x, 42, line_x + 16, 42, OLED_COLOR_NORMAL);

    OLED_ShowFrame();

    if (Key_IsSingleClick(KEY1_ID)) { // UP
        if (edit_idx == 0) val[0] = (val[0] + 1) % 24; // H
        if (edit_idx == 1) val[1] = (val[1] + 1) % 60; // M
        if (edit_idx == 2) val[2] = (val[2] + 1) % 60; // S
    }
    if (Key_IsSingleClick(KEY3_ID)) { // DOWN
        if (edit_idx == 0) val[0] = (val[0] == 0) ? 23 : val[0] - 1;
        if (edit_idx == 1) val[1] = (val[1] == 0) ? 59 : val[1] - 1;
        if (edit_idx == 2) val[2] = (val[2] == 0) ? 59 : val[2] - 1;
    }
    if (Key_IsSingleClick(KEY2_ID)) { // OK
        edit_idx++;
        if (edit_idx > 2) {
            Clock_SetTime(val[0], val[1], val[2]);
            ResetState();
            Menu_SwitchToMenu();
        }
    }
}

// ============================================================================
//   功能: 切换 12/24H (不切界面，只改状态)
// ============================================================================
void App_Toggle_Format(void) {
    // 1. 先获取旧状态
    TimeFormat old_fmt = Clock_GetFormat();
    
    // 2. 执行切换
    Clock_ToggleFormat();
    
    // 3. 弹窗提示
    OLED_NewFrame();
    OLED_DrawRectangle(10, 20, 108, 32, OLED_COLOR_NORMAL);
    OLED_DrawFilledRectangle(11, 21, 106, 30, OLED_COLOR_REVERSED);

    char buf[20];
    if (old_fmt == TIME_FMT_24H) {
        // 旧的是24，新的是12
        sprintf(buf, "24H -> 12H");
    } else {
        sprintf(buf, "12H -> 24H");
    }
    
    // 居中显示
    uint8_t x = (128 - strlen(buf)*6) / 2;
    OLED_PrintASCIIString(x, 32, buf, &afont12x6, OLED_COLOR_NORMAL);
    
    OLED_ShowFrame();
    HAL_Delay(800);
    Menu_SwitchToMenu();
}

// ============================================================================
//   功能: 调节亮度
// ============================================================================

// 内部变量：保存当前全局亮度 (默认最大)
// 因为 OLED_SetBrightness 内部有 static 缓存，但外部读不到
// 所以我们需要一个变量来记住当前设了多少。
static int16_t g_brightness = 255; 

// --- 亮度调节 APP ---
void App_Set_Brightness_Loop(void) {
    // 【修改1】使用静态变量保存当前调节的临时值
    // 使用 int16_t 是为了方便计算加减（防止 uint8_t 下溢出）
    static int16_t temp_bri = 0; 
    static uint8_t is_inited = 0;

    // 【修改2】初始化逻辑：每次进入APP时，从 Flash 参数中加载当前亮度
    if (!is_inited) {
        temp_bri = g_sys_params.brightness; 
        is_inited = 1;
    }

    // 1. 绘制 UI
    OLED_NewFrame();
    OLED_PrintASCIIString(0, 0, "Set Brightness", &afont12x6, OLED_COLOR_NORMAL);
    
    // 显示数值: "255"
    char buf[10];
    sprintf(buf, "%d", temp_bri); // 使用 temp_bri
    // 居中显示数值
    OLED_PrintASCIIString(64 - (strlen(buf)*8)/2, 20, buf, &afont16x8, OLED_COLOR_NORMAL);
    
    // 绘制进度条框 (X=14, Y=40, W=100, H=12)
    OLED_DrawRectangle(14, 40, 100, 12, OLED_COLOR_NORMAL);
    
    // 绘制内部填充条
    // 宽度 = (当前值 / 255) * (总宽-4)
    uint8_t bar_w = (uint16_t)temp_bri * 96 / 255;
    if (bar_w > 0) {
        OLED_DrawFilledRectangle(16, 42, bar_w, 8, OLED_COLOR_NORMAL);
    }
    
    OLED_ShowFrame();
    
    // 2. 按键逻辑
    // 每次调节步长为 15 (大概分 17 档)
    if (Key_IsSingleClick(KEY1_ID)) { // UP: 增加亮度
        temp_bri += 15;
        if (temp_bri > 255) temp_bri = 255;
        
        // 【核心】实时设置亮度以便预览，但此时不写 Flash
        OLED_SetBrightness(temp_bri); 
    }
    
    if (Key_IsSingleClick(KEY3_ID)) { // DOWN: 降低亮度
        temp_bri -= 15;
        if (temp_bri < 0) temp_bri = 0;
        
        // 【核心】实时设置亮度以便预览
        OLED_SetBrightness(temp_bri); 
    }
    
    if (Key_IsSingleClick(KEY2_ID)) { // OK: 确认保存并退出
        
        // 1. 更新全局结构体
        g_sys_params.brightness = (uint8_t)temp_bri;
        
        // 2. 【核心】写入 Flash
        System_Params_Save();
        
        // 3. 重置初始化标志并退出
        is_inited = 0;
        Menu_SwitchToMenu();
    }
}

void App_Set_Sleep_Loop(void) {
    // 选项列表
    const char *options[] = {"30 Sec", "1 Min", "5 Min", "Never"};
    const uint32_t values[] = {30000, 60000, 300000, 0};
    
    // 状态保持
    static uint8_t cursor = 0;
    static uint8_t inited = 0;
    
    // 初始化：第一次进入时，根据当前设置自动定位光标
    if (!inited) {
        // 从结构体回显光标位置
        uint32_t curr = g_sys_params.sleep_time;
        if (curr == 10000) cursor = 0;
        else if (curr == 30000) cursor = 1;
        else if (curr == 60000) cursor = 2;
        else cursor = 3;
        inited = 1;
    }

    // 绘制界面
    OLED_NewFrame();
    OLED_PrintASCIIString(20, 0, "Sleep Time", &afont12x6, OLED_COLOR_NORMAL);
    OLED_DrawLine(0, 12, 128, 12, OLED_COLOR_NORMAL);

    for (int i = 0; i < 4; i++) {
        uint8_t y = 16 + i * 12;
        
        if (i == cursor) {
            // 绘制选中项 (反色背景)
            OLED_DrawFilledRectangle(0, y, 128, 12, OLED_COLOR_NORMAL);
            OLED_PrintASCIIString(10, y + 2, (char*)options[i], &afont8x6, OLED_COLOR_REVERSED);
            // 画个小勾或者标记表示这是当前选中的
            OLED_PrintASCIIString(2, y + 2, ">", &afont8x6, OLED_COLOR_REVERSED);
        } else {
            // 普通项
            OLED_PrintASCIIString(10, y + 2, (char*)options[i], &afont8x6, OLED_COLOR_NORMAL);
        }
        
        // 标记当前生效的设置 (可选：在当前生效的选项后面画个星号 *)
        if (values[i] == Power_GetTimeout()) {
             // 如果不想太复杂，这一步可以省略，因为光标默认就是当前项
        }
    }
    OLED_ShowFrame();

    // 按键处理
    if (Key_IsSingleClick(KEY1_ID)) { // UP
        if (cursor > 0) cursor--;
        else cursor = 3;
    }
    if (Key_IsSingleClick(KEY3_ID)) { // DOWN
        if (cursor < 3) cursor++;
        else cursor = 0;
    }
    if (Key_IsSingleClick(KEY2_ID)) { // OK
        // 应用设置
        Power_SetTimeout(values[cursor]);
        
        // 退出
        inited = 0; // 重置初始化标志，方便下次进入
        Menu_SwitchToMenu();
    }
}

void App_Set_Sound_Loop(void) {
    // 选项文本
    const char *options[] = {"Sound ON", "Sound OFF (Mute)"};
    
    // 状态保持
    static uint8_t cursor = 0;
    static uint8_t inited = 0;
    
    // 初始化：第一次进入时，读取当前硬件状态来设置光标
    if (!inited) {
        bool is_muted = MP3_GetMuteStatus();
        if (!inited) {
        // 0=ON, 1=OFF/Mute
        cursor = g_sys_params.volume_mute ? 1 : 0; 
        inited = 1;
    }

        inited = 1;
    }

    // 1. 绘制界面
    OLED_NewFrame();
    OLED_PrintASCIIString(20, 0, "Sound Setting", &afont12x6, OLED_COLOR_NORMAL);
    OLED_DrawLine(0, 12, 128, 12, OLED_COLOR_NORMAL);

    for (int i = 0; i < 2; i++) {
        uint8_t y = 24 + i * 16;
        
        if (i == cursor) {
            // 选中项反色
            OLED_DrawFilledRectangle(0, y, 128, 14, OLED_COLOR_NORMAL);
            OLED_PrintASCIIString(10, y + 2, (char*)options[i], &afont8x6, OLED_COLOR_REVERSED);
            OLED_PrintASCIIString(2, y + 2, ">", &afont8x6, OLED_COLOR_REVERSED);
        } else {
            // 未选中项
            OLED_PrintASCIIString(10, y + 2, (char*)options[i], &afont8x6, OLED_COLOR_NORMAL);
        }
        
        // 可选：在当前生效的选项后面打个勾
        bool current_mute_state = MP3_GetMuteStatus();
        // i=0(ON) 且 mute=false -> 勾
        // i=1(OFF) 且 mute=true -> 勾
        if ((i == 0 && !current_mute_state) || (i == 1 && current_mute_state)) {
            OLED_PrintASCIIString(110, y + 2, "*", &afont8x6, (i==cursor)?OLED_COLOR_REVERSED:OLED_COLOR_NORMAL);
        }
    }
    OLED_ShowFrame();

    // 2. 按键处理
    if (Key_IsSingleClick(KEY1_ID)) { // UP
        if (cursor > 0) cursor--;
        else cursor = 1;
    }
    if (Key_IsSingleClick(KEY3_ID)) { // DOWN
        if (cursor < 1) cursor++;
        else cursor = 0;
    }
    if (Key_IsSingleClick(KEY2_ID)) { // OK
        // 执行设置
        if (cursor == 0) {
            MP3_SetMuteStatus(false); // Sound ON -> Unmute
        } else {
            MP3_SetMuteStatus(true);  // Sound OFF -> Mute
        }
        
        // 退出
        inited = 0;
        Menu_SwitchToMenu();
    }
}


// ============================================================================
//   系统信息 App (System Info)
// ============================================================================

// 定义总行数（根据你要显示的内容行数）
#define SYS_INFO_LINES  8
#define VISIBLE_LINES   3   // 【修改】一页显示 3 行
#define LINE_HEIGHT     16  // 【修改】行高 16 px

static uint8_t scroll_top = 0; 
static float real_time_temp = 0.0f;
static uint32_t last_temp_tick = 0;

void App_System_Info_Loop(void) {
    // 1. 定时更新温度
    if (HAL_GetTick() - last_temp_tick > 500) {
        real_time_temp = Get_MCU_Temperature();
        last_temp_tick = HAL_GetTick();
    }

    // 2. 准备显示内容 (使用中文前缀)
    // 缓冲区稍微大一点，防止溢出
    char content[SYS_INFO_LINES][32]; 
    
    // 注意：这里的汉字编码必须与你 Flash 里的字库编码一致（通常是 GBK）
    sprintf(content[0], "设备：STM32手表"); 
    sprintf(content[1], "核心：F103C8T6");
    sprintf(content[2], "存储：20K+64K");
    sprintf(content[3], "闪存：16MB ext");
    sprintf(content[4], "主频：72 MHz");
    sprintf(content[5], "电池：250mAh");
    sprintf(content[6], "版本：V3.0 hjh");
    sprintf(content[7], "温度：%.1f C", real_time_temp);

    // 3. 绘制界面
    OLED_NewFrame();
    
    // --- 顶部标题栏 (保持原样，或改为中文 "系统信息") ---
    OLED_DrawFilledRectangle(0, 0, 14, 12, OLED_COLOR_NORMAL);
    OLED_PrintASCIIString(1, 2, "<<", &afont8x6, OLED_COLOR_REVERSED);
    // 这里依然用英文标题，为了省空间，或者你可以用 OLED_ShowGBK 写 "系统信息"
    OLED_PrintASCIIString(20, 1, "System Info", &afont12x6, OLED_COLOR_NORMAL);
    
    // 分割线 (Y=14)
    OLED_DrawLine(0, 14, 128, 14, OLED_COLOR_NORMAL);

    // --- 内容区域 (可滚动) ---
    // 标题占用 0-14，内容从 Y=16 开始
    uint8_t start_y = 16;

    for (int i = 0; i < VISIBLE_LINES; i++) {
        uint8_t data_idx = scroll_top + i;
        if (data_idx >= SYS_INFO_LINES) break;
        uint8_t y_pos = start_y + i * LINE_HEIGHT;
        OLED_ShowGBK(0, y_pos, content[data_idx], 16, OLED_COLOR_NORMAL);
    }
    
    // --- 滚动条指示器 ---
    // 进度条高度 = 总高度48 * (可见3行 / 总8行)
    uint8_t total_h = 48;
    uint8_t bar_h = total_h * VISIBLE_LINES / SYS_INFO_LINES;
    if(bar_h < 4) bar_h = 4; // 最小高度
    uint8_t bar_y = start_y + (scroll_top * (total_h - bar_h) / (SYS_INFO_LINES - VISIBLE_LINES));
    
    if (SYS_INFO_LINES > VISIBLE_LINES) {
        OLED_DrawLine(127, start_y, 127, 63, OLED_COLOR_NORMAL); // 轨道
        OLED_DrawFilledRectangle(125, bar_y, 3, bar_h, OLED_COLOR_NORMAL); // 滑块
    }

    OLED_ShowFrame();

    // 4. 按键处理
    if (Key_IsSingleClick(KEY3_ID)) { // UP
        if (scroll_top > 0) scroll_top--;
    }
    
    if (Key_IsSingleClick(KEY1_ID)) { // DOWN
        if (scroll_top < SYS_INFO_LINES - VISIBLE_LINES) scroll_top++;
    }
    
    if (Key_IsSingleClick(KEY2_ID)) { // OK -> 退出
        scroll_top = 0;
        Menu_SwitchToMenu();
    }
}