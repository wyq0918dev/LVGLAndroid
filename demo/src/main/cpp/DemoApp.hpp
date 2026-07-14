#pragma once

#include <atomic>
#include <thread>
#include <android/log.h>
#include <android/native_window.h>
#include <lvgl.h>

#ifndef LOG_TAG
#define LOG_TAG "lvgl_demo"
#endif

#ifndef NO_LOG
#ifndef NO_DEBUG_LOG
#define LOG_D(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#else
#define LOG_D(...)
#endif
#define LOG_V(...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#define LOG_I(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOG_W(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOG_E(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#else
#define LOG_D(...)
#define LOG_V(...)
#define LOG_I(...)
#define LOG_W(...)
#define LOG_E(...)
#endif

// LVGL 引擎：管理渲染线程、显示刷新与触摸输入
class DemoApp {

public:
    ~DemoApp();

    // 绑定 NativeWindow 并启动渲染线程
    void StartRender(ANativeWindow *Window);

    // 接收触摸（Touch=true 按下），将屏幕坐标映射为逻辑坐标
    void OnTouch(bool Touch, int X, int Y);

    // 停止渲染线程并等待退出
    void StopRender();

private:
    ANativeWindow *Window = nullptr;          // 原生窗口
    ANativeWindow_Buffer WindowBuffer;        // 帧缓冲信息
    const int AppWidth = 480;                 // LVGL 逻辑宽
    const int AppHeight = 320;                // LVGL 逻辑高
    int ScreenWidth = 0;                      // 物理屏宽（坐标映射）
    int ScreenHeight = 0;                     // 物理屏高（坐标映射）
    uint32_t *SurfaceBuffer = nullptr;        // 全帧累积缓冲（解决多缓冲闪烁）
    size_t SurfaceSize = 0;                   // 累积缓冲大小
    std::atomic<bool> bIsTouch = false;       // 触摸状态
    std::atomic<int> TouchX = 0;              // 触摸 X（逻辑坐标）
    std::atomic<int> TouchY = 0;              // 触摸 Y（逻辑坐标）
    std::atomic<bool> bIsRunning = false;     // 渲染线程运行标志
    std::thread RenderThread;                 // 渲染线程

    // 刷新回调：RGB565 → RGBA_8888，累积整帧后写入窗口
    void LvFlushCallback(lv_display_t *Display, const lv_area_t *Area, uint8_t *PxMap);

    // 触摸回调：向 LVGL 上报当前触摸状态与坐标
    void LvTouchCallback(lv_indev_t *IndevDriver, lv_indev_data_t *Data);

    // 渲染线程主循环
    void LvLoopTask();

    // 心跳：返回毫秒时间戳
    static uint32_t LvTickGet();

    // 日志：转发到 logcat
    static void LvLogPrint(lv_log_level_t Level, const char *Buf);

    // 静态刷新回调包装（C 回调 → 实例方法）
    static void LvFlushCbStatic(lv_display_t *Display, const lv_area_t *Area, uint8_t *PxMap);

    // 静态触摸回调包装（C 回调 → 实例方法）
    static void LvTouchCbStatic(lv_indev_t *IndevDriver, lv_indev_data_t *Data);
};
