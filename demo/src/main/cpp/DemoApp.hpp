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

using namespace std;

/**
 * LVGL 应用核心类，管理 LVGL 在 Android 上的完整生命周期。
 *
 * 线程安全说明：
 *   - isTouch/touchX/touchY/bIsRunning 使用 atomic 保证跨线程读写安全
 *   - LVGL API 调用全部在渲染线程内完成，不需要额外锁
 *   - start/stop 方法通过 join 保证线程同步
 */
class DemoApp {

public:
    /** 析构函数，停止渲染线程并释放资源 */
    ~DemoApp();

    /** 启动 LVGL 渲染，在独立线程中运行主循环 */
    void start(ANativeWindow *window);

    /** 处理触摸事件，将屏幕坐标映射为 LVGL 逻辑坐标 */
    void onTouch(int touch, int x, int y);

    /** 停止渲染线程，等待线程退出 */
    void stop();

private:
    ANativeWindow *window = nullptr;          // Android 原生窗口引用
    ANativeWindow_Buffer windowBuffer;         // NativeWindow 帧缓冲信息
    int app_width = 480, app_height = 320;     // LVGL 逻辑分辨率
    int screen_width = 0, screen_height = 0;   // 物理屏幕尺寸（用于触摸坐标映射）
    uint32_t *surface_buf = nullptr;           // 全帧累积缓冲区（RGBA_8888，4 字节/像素），
    // ANativeWindow（尤其是 TextureView 的 SurfaceTexture）
    // 以 RGBA_8888 作为消费者格式，需将 LVGL 的
    // RGB565 绘制结果转换后写入；多缓冲机制下每次
    // 锁定可能返回不同缓冲区，需持久缓冲区累积
    // 局部刷新后整体拷贝
    size_t surface_size = 0;                    // 全帧累积缓冲区大小
    atomic<int> isTouch = 0;                   // 触摸状态：1=按下，0=释放
    atomic<int> touchX = 0;                    // 触摸 X 坐标（已映射为 LVGL 逻辑坐标）
    atomic<int> touchY = 0;                    // 触摸 Y 坐标（已映射为 LVGL 逻辑坐标）
    atomic<bool> bIsRunning = false;           // 渲染线程运行标志
    thread m_thread;                       // LVGL 渲染线程

    // LVGL 显示刷新回调（实例方法），将局部区域像素写入 NativeWindow
    void lv_flush_callback(lv_display_t *display, const lv_area_t *area, uint8_t *px_map);

    // LVGL 触摸输入读取回调（实例方法），向 LVGL 报告当前触摸状态
    void lv_touch_callback(lv_indev_t *indev_drv, lv_indev_data_t *data);

    // LVGL 主循环任务，运行在独立线程中
    void lv_loop_task();

    // LVGL tick 回调，返回当前系统时间戳（毫秒）
    static uint32_t lv_tick_get();

    // LVGL 日志回调，转发到 Android logcat
    static void lv_log_print(lv_log_level_t level, const char *buf);

    // 静态刷新回调包装，通过 user_data 转发到实例方法
    static void lv_flush_cb_static(lv_display_t *display, const lv_area_t *area, uint8_t *px_map);

    // 静态触摸回调包装，通过 user_data 转发到实例方法
    static void lv_touch_cb_static(lv_indev_t *indev_drv, lv_indev_data_t *data);
};
