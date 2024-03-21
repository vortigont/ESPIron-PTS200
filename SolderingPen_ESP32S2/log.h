/*
LOG macro will enable/disable logs to serial depending on LAMP_DEBUG build-time flag
*/
#pragma once

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
	#define LOGV(tag, func, ...) PTS200_DEBUG_PORT.print("V: "); PTS200_DEBUG_PORT.print(tag); PTS200_DEBUG_PORT.print("\t"); PTS200_DEBUG_PORT.func(__VA_ARGS__)
#else
	#define LOGV(...)
#endif

#if defined(PTS200_DEBUG_LEVEL) && PTS200_DEBUG_LEVEL > 3
	#define LOGD(tag, func, ...) PTS200_DEBUG_PORT.print("D: "); PTS200_DEBUG_PORT.print(tag); PTS200_DEBUG_PORT.print("\t"); PTS200_DEBUG_PORT.func(__VA_ARGS__)
#else
	#define LOGD(...)
#endif

#if defined(PTS200_DEBUG_LEVEL) && PTS200_DEBUG_LEVEL > 2
	#define LOGI(tag, func, ...) PTS200_DEBUG_PORT.print("I: "); PTS200_DEBUG_PORT.print(tag); PTS200_DEBUG_PORT.print("\t"); PTS200_DEBUG_PORT.func(__VA_ARGS__)
	// compat macro
	#define LOG(func, ...) PTS200_DEBUG_PORT.func(__VA_ARGS__)
#else
	#define LOGI(...)
	// compat macro
	#define LOG(...)
#endif

#if defined(PTS200_DEBUG_LEVEL) && PTS200_DEBUG_LEVEL > 1
	#define LOGW(tag, func, ...) PTS200_DEBUG_PORT.print("W: "); PTS200_DEBUG_PORT.print(tag); PTS200_DEBUG_PORT.print("\t"); PTS200_DEBUG_PORT.func(__VA_ARGS__)
#else
	#define LOGW(...)
#endif

#if defined(PTS200_DEBUG_LEVEL) && PTS200_DEBUG_LEVEL > 0
	#define LOGE(tag, func, ...) PTS200_DEBUG_PORT.print("E: "); PTS200_DEBUG_PORT.print(tag); PTS200_DEBUG_PORT.print("\t"); PTS200_DEBUG_PORT.func(__VA_ARGS__)
#else
	#define LOGE(...)
#endif
