#pragma once
// 菜单项
static const uint8_t language_types = 3;
//                                            "温控类型",   "溫控類型",   "Control Type",

static const char *SetupItems[][language_types] = { "菜单设置", "菜單設置", "Setup Menu",
                                             "烙铁头设置", "烙鐵頭設置", "Tip Settings",
                                             "温度设置", "溫度設置", "Temp Settings",
                                             "时间设置", "時間設置", "Timer Settings",
                                             "主屏幕", "主屏幕", "Main Screen",
                                             "信息", "信息", "Information",
                                             "电压设置", "電壓設置", "Voltage",
                                             "QC", "QC", "QC",
                                             "蜂鸣器", "蜂鳴器", "Buzzer",
                                             "恢复默认设置", "恢復默認設置", "Restore Config",
                                             "更新版本", "更新版本", "Update Firmware",
                                             "Languages", "Languages", "Languages",
                                             "切换左右手", "切换左右手", "L/R Hand",
                                             "返回", "返回", "Return" };
static const char *LanguagesItems[][language_types] = { "Languages", "Languages", "Languages",
                                                 "简体中文", "简体中文", "zh-CN",
                                                 "繁体中文", "繁体中文", "zh-TW",
                                                 "English", "English", "English" };
static const char *TipItems[][language_types] = { "烙铁头:", "烙鐵頭:", "Tip:",
                                           "更换烙铁头", "更換烙鐵頭", "Change Tip",
                                           "校准烙铁头", "校準烙鐵頭", "Calibrate Tip",
                                           "重命名烙铁头", "重命名烙鐵頭", "Rename Tip",
                                           "删除烙铁头", "刪除烙鐵頭", "Delete Tip",
                                           "新建烙铁头", "新建烙鐵頭", "New Tip",
                                           "返回", "返回", "Return" };
static const char *TempItems[][language_types] = { "温度设置", "溫度設置", "Temp Settings",
                                            "默认温度", "默認溫度", "Default Temp",
                                            "休眠温度", "休眠溫度", "Sleep Temp",
                                            "提高温度", "提高溫度", "Boost Temp",
                                            "返回", "返回", "Return" };
static const char *TimerItems[][language_types] = { "时间设置", "時間設置", "Timer Settings",
                                             "休眠时间", "休眠時間", "Sleep Timer",
                                             "关闭时间", "關閉時間", "Shutdown Timer",
                                             "提温时间", "提溫時間", "Boost Timer",
                                             "唤醒阈值", "唤醒閾值", "Wake Threshold",
                                             "返回", "返回", "Return" };
static const char *ControlTypeItems[][language_types] = { "温控类型", "溫控類型", "Control Type",
                                                   "非PID", "非PID", "Non-PID",
                                                   "PID", "PID", "PID" };
static const char *MainScreenItems[][language_types] = {
  "主屏幕",
  "主屏幕",
  "Main Screen",
  "大数字",
  "大數字",
  "Big Number",
  "更多信息",
  "更多信息",
  "More Info",
};
static const char *StoreItems[][language_types] = { "存储设置?", "存儲設置?", "Save?",
                                             "否", "否", "No",
                                             "是", "是", "Yes" };
static const char *DefaultItems[][language_types] = { "恢复设置?", "恢復設置?", "Restore?",
                                               "否", "否", "No",
                                               "是", "是", "Yes" };
static const char *SureItems[][language_types] = { "确定?", "確定?", "Sure?",
                                            "否", "否", "No",
                                            "是", "是", "Yes" };
static const char *VoltageItems[][language_types] = { "电压设置", "電壓設置", "Voltage Settings",
                                               "9V", "9V", "9V",
                                               "12V", "12V", "12V",
                                               "15V", "15V", "15V",
                                               "20V(50%)", "20V(50%)", "20V(50%)",
                                               "20V(100%)", "20V(100%)", "20V(100%)"};
static const char *QCItems[][language_types] = { "QC", "QC", "QC",
                                          "禁用", "禁用", "Disable",
                                          "启用", "啟用", "Enable" };
static const char *BuzzerItems[][language_types] = { "蜂鸣器", "蜂鳴器", "Buzzer",
                                              "禁用", "禁用", "Disable",
                                              "启用", "啟用", "Enable" };
static const char *DefaultTempItems[][language_types] = { "默认温度", "默認溫度", "Default Temp",
                                                   "deg C", "deg C", "deg C" };
static const char *SleepTempItems[][language_types] = { "休眠温度", "休眠溫度", "Sleep Temp",
                                                 "deg C", "deg C", "deg C" };
static const char *BoostTempItems[][language_types] = { "提高温度", "提高溫度", "Boost Temp",
                                                 "deg C", "deg C", "deg C" };
static const char *SleepTimerItems[][language_types] = { "休眠时间", "休眠時間", "Sleep Timer",
                                                  "秒", "秒", "sec" };
static const char *WAKEUPthresholdItems[][language_types] = { "唤醒阈值", "唤醒閾值", "Wake Threshold",
                                                       "mg", "mg", "mg" };
static const char *OffTimerItems[][language_types] = { "关闭时间", "關閉時間", "Shutdown Timer",
                                                "分钟", "分鐘", "min" };
static const char *BoostTimerItems[][language_types] = { "提温时间", "提溫時間", "Boost Timer",
                                                  "秒", "秒", "sec" };
static const char *DeleteMessage[][language_types] = { "警告", "警告", "Warning",
                                                "你不能", "你不能", "You can't",
                                                "删除你的", "刪除你的", "delete your",
                                                "最后的烙铁头!", "最後的烙鐵頭!", "last tip!" };
static const char *MaxTipMessage[][language_types] = { "警告", "警告", "Warning",
                                                "你已达", "你已達", "You reached",
                                                "最大数量", "最大數量", "maximum number",
                                                "的烙铁头!", "的烙鐵頭!", "of tips!" };

static const char *txt_set_temp[] = { "设温:", "設溫:", "SET:" };
static const char *txt_error[] = { "错误", "錯誤", "ERROR" };
static const char *txt_off[] = { "关闭", "關閉", "OFF" };
static const char *txt_sleep[] = { "休眠", "休眠", "SLEEP" };
static const char *txt_boost[] = { "提温", "提溫", "BOOST" };
static const char *txt_worky[] = { "工作", "工作", "WORK" };
static const char *txt_on[] = { "加热", "加熱", "HEAT" };
static const char *txt_hold[] = { "保持", "保持", "HOLD" };

static const char *txt_Deactivated[] = { "禁用", "禁用", "Deactivated" };

static const char *txt_temp[] = { "温度:", "溫度:", "TEMP:" };
static const char *txt_voltage[] = { "电压:", "電壓:", "VOLT:" };
static const char *txt_Version[] = { "版本:", "版本:", "VER:" };

static const char *txt_select_tip[] = { "选择烙铁头", "選擇烙鐵頭", "Select Tip" };

static const char *txt_calibrate[] = { "校准", "校準", "Calibrate" };
static const char *txt_step[] = { "步进", "步進", "Step" };
static const char *txt_set_measured[] = { "设为测量", "設為測量", "Set Measure" };
static const char *txt_s_temp[] = { "的温度:", "的溫度:", "Temp:" };
static const char *txt_temp_2[] = { "温度：  ", "溫度：  ", "Temp:  " };
static const char *txt_wait_pls[] = { "请稍等...", "請稍等...", "Please wait ..." };

static const char *txt_enter_tip_name[] = { "输入烙铁头名称", "輸入烙鐵頭名稱", "Enter Tip Name" };
