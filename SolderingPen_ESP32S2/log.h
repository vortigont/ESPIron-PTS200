/*
LOG macro will enable/disable logs to serial depending on LAMP_DEBUG build-time flag
*/
#pragma once

static constexpr const char* S_V = "V: ";
static constexpr const char* S_D = "D: ";
static constexpr const char* S_I = "I: ";
static constexpr const char* S_W = "W: ";
static constexpr const char* S_E = "E: ";

#ifndef PTS200_DEBUG_PORT
#define PTS200_DEBUG_PORT Serial
#endif

// undef possible LOG macros
#ifdef LOG
  #undef LOG
#endif
#ifdef LOGV
  #undef LOGV
#endif
#ifdef LOGD
  #undef LOGD
#endif
#ifdef LOGI
  #undef LOGI
#endif
#ifdef LOGW
  #undef LOGW
#endif
#ifdef LOGE
  #undef LOGE
#endif

#if defined(PTS200_DEBUG_LEVEL) && PTS200_DEBUG_LEVEL == 5
	#define LOGV(tag, func, ...) PTS200_DEBUG_PORT.print(S_V); PTS200_DEBUG_PORT.print(tag); PTS200_DEBUG_PORT.print((char)0x9); PTS200_DEBUG_PORT.func(__VA_ARGS__)
#else
	#define LOGV(...)
#endif

#if defined(PTS200_DEBUG_LEVEL) && PTS200_DEBUG_LEVEL > 3
	#define LOGD(tag, func, ...) PTS200_DEBUG_PORT.print(S_D); PTS200_DEBUG_PORT.print(tag); PTS200_DEBUG_PORT.print((char)0x9); PTS200_DEBUG_PORT.func(__VA_ARGS__)
#else
	#define LOGD(...)
#endif

#if defined(PTS200_DEBUG_LEVEL) && PTS200_DEBUG_LEVEL > 2
	#define LOGI(tag, func, ...) PTS200_DEBUG_PORT.print(S_I); PTS200_DEBUG_PORT.print(tag); PTS200_DEBUG_PORT.print((char)0x9); PTS200_DEBUG_PORT.func(__VA_ARGS__)
	// compat macro
	#define LOG(func, ...) PTS200_DEBUG_PORT.func(__VA_ARGS__)
#else
	#define LOGI(...)
	// compat macro
	#define LOG(...)
#endif

#if defined(PTS200_DEBUG_LEVEL) && PTS200_DEBUG_LEVEL > 1
	#define LOGW(tag, func, ...) PTS200_DEBUG_PORT.print(S_W); PTS200_DEBUG_PORT.print(tag); PTS200_DEBUG_PORT.print((char)0x9); PTS200_DEBUG_PORT.func(__VA_ARGS__)
#else
	#define LOGW(...)
#endif

#if defined(PTS200_DEBUG_LEVEL) && PTS200_DEBUG_LEVEL > 0
	#define LOGE(tag, func, ...) PTS200_DEBUG_PORT.print(S_E); PTS200_DEBUG_PORT.print(tag); PTS200_DEBUG_PORT.print((char)0x9); PTS200_DEBUG_PORT.func(__VA_ARGS__)
#else
	#define LOGE(...)
#endif

// Per app macros
#if defined(ADC_DEBUG_LEVEL) && ADC_DEBUG_LEVEL == 5
	#define ADC_LOGV(func, ...) PTS200_DEBUG_PORT.print(S_V); PTS200_DEBUG_PORT.print(T_ADC); PTS200_DEBUG_PORT.print((char)0x9); PTS200_DEBUG_PORT.func(__VA_ARGS__)
#else
	#define ADC_LOGV(...)
#endif

#if defined(CTRL_DEBUG_LEVEL) && CTRL_DEBUG_LEVEL == 5
	#define CTRL_LOGV(func, ...) PTS200_DEBUG_PORT.print(S_V); PTS200_DEBUG_PORT.print(T_CTRL); PTS200_DEBUG_PORT.print((char)0x9); PTS200_DEBUG_PORT.func(__VA_ARGS__)
#else
	#define CTRL_LOGV(...)
#endif

#if defined(PWM_DEBUG_LEVEL) && PWM_DEBUG_LEVEL == 5
	#define PWM_LOGV(func, ...) PTS200_DEBUG_PORT.print(S_V); PTS200_DEBUG_PORT.print(T_PWM); PTS200_DEBUG_PORT.print((char)0x9); PTS200_DEBUG_PORT.func(__VA_ARGS__)
#else
	#define PWM_LOGV(...)
#endif
