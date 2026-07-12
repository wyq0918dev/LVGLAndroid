#pragma once

#include <android/log.h>

#ifndef LVGL_DEMO_LOG_TAG
#define LVGL_DEMO_LOG_TAG "lvgl_demo"
#endif

#ifndef NO_LOG
#ifndef NO_DEBUG_LOG
#define LOG_D(...) __android_log_print(ANDROID_LOG_DEBUG, LVGL_DEMO_LOG_TAG, __VA_ARGS__)
#else
#define LOG_D(...)
#endif
#define LOG_V(...) __android_log_print(ANDROID_LOG_VERBOSE, LVGL_DEMO_LOG_TAG, __VA_ARGS__)
#define LOG_I(...) __android_log_print(ANDROID_LOG_INFO, LVGL_DEMO_LOG_TAG, __VA_ARGS__)
#define LOG_W(...) __android_log_print(ANDROID_LOG_WARN, LVGL_DEMO_LOG_TAG, __VA_ARGS__)
#define LOG_E(...) __android_log_print(ANDROID_LOG_ERROR, LVGL_DEMO_LOG_TAG, __VA_ARGS__)
#else
#define LOG_D(...)
#define LOG_V(...)
#define LOG_I(...)
#define LOG_W(...)
#define LOG_E(...)
#endif