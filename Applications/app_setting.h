#ifndef __APP_SETTING_H
#define __APP_SETTING_H
void App_System_Info_Loop(void);

// 设置应用的入口函数
void App_Set_Date_Loop(void); // 日期设置
void App_Set_Time_Loop(void); // 时间设置

// 格式切换功能 (可以直接被菜单项调用)
void App_Toggle_Format(void);
// 亮度调节 APP
void App_Set_Brightness_Loop(void);
void App_Set_Sleep_Loop(void); 
void App_Set_Sound_Loop(void);

#endif