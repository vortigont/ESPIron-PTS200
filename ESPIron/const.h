#pragma once

// static literals are defined here

// LOG tags
static constexpr const char* T_ADC = "ADC";
static constexpr const char* T_CTRL = "CTRL";
static constexpr const char* T_DBG = "DBG";
static constexpr const char* T_GYRO = "GYRO";
static constexpr const char* T_PWM = "PWM";
static constexpr const char* T_HEAT = "HEAT";

// NVS namespaces
static constexpr const char* T_IRON = "IRON";
static constexpr const char* T_Sensor = "Sensor";
static constexpr const char* T_UI = "UI";
static constexpr const char* T_HID = "HID";

// NVS keys
static constexpr const char* T_timeouts = "timeouts";                   // blob with timeout values
static constexpr const char* T_temperatures = "temperatures";           // blob with temperature values
static constexpr const char* T_motionThr = "motionThr";                 // motion threshold (uint32)
static constexpr const char* T_pdVolts = "pdVolts";                     // PD trigger voltage
static constexpr const char* T_qcVolts = "qcVolts";                     // QC trigger voltage
static constexpr const char* T_qcMode = "qcMode";                       // QC Mode
static constexpr const char* T_PWMRamp = "PWMRamp";                     // PWM Power ramping
