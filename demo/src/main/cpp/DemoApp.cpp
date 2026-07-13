#include "DemoApp.hpp"
#include <android/native_window_jni.h>
#include <jni.h>
#include <lv_demos.h>
#include <unistd.h>

using namespace std;

// ===================== DemoApp 实现 =====================

/**
 * LVGL 显示刷新回调（实例方法）。
 *
 * LVGL 在 PARTIAL 渲染模式下，每次完成一块区域的绘制后会调用此回调。
 * 回调负责将 px_map 中的局部像素数据写入 ANativeWindow 的帧缓冲。
 *
 * 实现流程：
 *   1. 懒分配 surface_buf（与 NativeWindow 帧缓冲等大的全帧累积缓冲）
 *   2. 将局部区域逐行拷贝到 surface_buf 对应位置（累积完整帧）
 *   3. 锁定 NativeWindow，将 surface_buf 整体拷贝到帧缓冲，解锁并提交
 *   4. 通知 LVGL 刷新完成（lv_disp_flush_ready）
 *
 * 注意：ANativeWindow 使用多缓冲机制，每次 lock 可能返回不同缓冲区。
 * 必须用 surface_buf 累积完整帧后整体拷贝，否则部分缓冲区只收到局部更新
 * 会导致显示闪烁或残缺。
 *
 * @param display  LVGL 显示对象
 * @param area  本次刷新的矩形区域（LVGL 逻辑坐标）
 * @param px_map LVGL 绘制缓冲，包含 area 区域内的像素数据（RGB565 格式）
 */
void DemoApp::lv_flush_callback(lv_display_t *display, const lv_area_t *area, uint8_t *px_map) {
    if (bIsRunning && window != nullptr) {
        // 懒分配全帧累积缓冲区（RGBA_8888，4 字节/像素），大小与 NativeWindow 帧缓冲一致
        if (surface_buf == nullptr) {
            surface_size = sizeof(uint32_t) * windowBuffer.stride * windowBuffer.height;
            surface_buf = (uint32_t *) malloc(surface_size);
        }
        int w = area->x2 - area->x1 + 1;
        int h = area->y2 - area->y1 + 1;
        auto *src = (uint16_t *) px_map;        // LVGL 绘制缓冲为 RGB565
        auto stride = windowBuffer.stride;
        // 将局部区域 RGB565 逐像素转换为 RGBA_8888 后写入累积缓冲对应位置
        for (int i = 0; i < h; i++) {
            auto *dst = &surface_buf[(area->y1 + i) * stride + area->x1];
            auto *src_line = &src[i * w];
            for (int j = 0; j < w; j++) {
                uint16_t c = src_line[j];
                auto r = (uint8_t) (((c >> 11) & 0x1F) * 255 / 31);
                auto g = (uint8_t) (((c >> 5) & 0x3F) * 255 / 63);
                auto b = (uint8_t) ((c & 0x1F) * 255 / 31);
                // 小端内存布局：A(高字节) B G R(低字节)
                dst[j] = (0xFFu << 24) | ((uint32_t) b << 16) | ((uint32_t) g << 8) | r;
            }
        }
        // 锁定帧缓冲，整体拷贝后提交显示
        if (ANativeWindow_lock(window, &windowBuffer, nullptr) == 0) {
            memcpy(windowBuffer.bits, surface_buf, surface_size);
            ANativeWindow_unlockAndPost(window);
        }
    }
    // 通知 LVGL 本次刷新已完成，可以继续下一帧
    lv_disp_flush_ready(display);
}

/**
 * LVGL 触摸输入读取回调（实例方法）。
 *
 * LVGL 主循环每次调用 lv_timer_handler 时会定期读取输入设备状态。
 * 此回调将 Java 层转发来的触摸坐标和按下/释放状态传递给 LVGL。
 *
 * @param indev_drv LVGL 输入设备对象
 * @param data      输出参数，填充当前触摸状态和坐标
 */
void DemoApp::lv_touch_callback(lv_indev_t *indev_drv, lv_indev_data_t *data) {
    if (isTouch) {
        data->point.x = (short) touchX;
        data->point.y = (short) touchY;
        data->state = LV_INDEV_STATE_PR;  // 按下状态
    } else {
        data->state = LV_INDEV_STATE_REL;  // 释放状态
    }
}

/**
 * LVGL 日志回调，将 LVGL 内部日志转发到 Android logcat。
 *
 * @param level 日志级别
 * @param buf   日志内容
 */
void DemoApp::lv_log_print(lv_log_level_t level, const char *buf) {
    switch (level) {
        case LV_LOG_LEVEL_INFO:
            LOG_I("%s", buf);
            break;
        case LV_LOG_LEVEL_WARN:
            LOG_W("%s", buf);
            break;
        case LV_LOG_LEVEL_ERROR:
            LOG_E("%s", buf);
            break;
        case LV_LOG_LEVEL_TRACE:
            LOG_D("%s", buf);
            break;
        default:
            LOG_I("%s", buf);
    }
}

/**
 * LVGL tick 获取回调，返回当前系统时间戳（毫秒）。
 * 使用 gettimeofday 获取高精度时间，作为 LVGL 的心跳源。
 *
 * @return 当前时间戳（毫秒）
 */
uint32_t DemoApp::lv_tick_get() {
    static struct timeval tv;
    gettimeofday(&tv, nullptr);
    return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

/**
 * 启动 LVGL 渲染。
 *
 * 如果已有渲染线程在运行，先停止并等待退出。然后设置 NativeWindow 的几何属性
 * （分辨率和像素格式为 RGBA_8888），最后在独立线程中启动 LVGL 主循环。
 *
 * @param win 由 ANativeWindow_fromSurface 获取的 NativeWindow 引用，
 *            调用者负责在渲染线程退出时 release
 */
void DemoApp::start(ANativeWindow *win) {
    stop();  // 确保之前的渲染线程已退出
    if (win == nullptr) {
        return;
    }
    window = win;
    // 记录物理屏幕尺寸，用于触摸坐标映射
    screen_width = ANativeWindow_getWidth(window);
    screen_height = ANativeWindow_getHeight(window);
    // 设置 NativeWindow 缓冲区几何属性：使用 LVGL 逻辑分辨率 + RGBA_8888 格式。
    // 采用 RGBA_8888 而非 RGB_565，是因为 TextureView 的 SurfaceTexture 作为消费者
    // 以 RGBA_8888 采样 GL 纹理，RGB_565 在部分设备上会显示错乱；该格式对 SurfaceView
    // 同样兼容。LVGL 绘制缓冲仍为 RGB565，在刷新回调中转换为 RGBA_8888 写入窗口。
    if (ANativeWindow_setBuffersGeometry(window, app_width, app_height, WINDOW_FORMAT_RGBA_8888) != 0) {
        LOG_E("Failed to set NativeWindow geometry [%d x %d]", app_width, app_height);
        ANativeWindow_release(window);
        window = nullptr;
        return;
    }
    LOG_D("LV Screen [%d x %d]", app_width, app_height);
    bIsRunning = true;
    m_thread = thread(&DemoApp::lv_loop_task, this);
}

/**
 * LVGL 主循环任务，运行在独立的渲染线程中。
 *
 * 执行流程：
 *   1. 初始化 LVGL 核心（lv_init）、设置 tick 和日志回调
 *   2. 创建显示对象，绑定刷新回调和绘制缓冲
 *   3. 创建触摸输入设备，绑定读取回调
 *   4. 直接启动 AppList 映射表中 "default" 对应的 Demo
 *   5. 进入主循环，定期调用 lv_timer_handler 驱动 LVGL 刷新
 *   6. 收到停止信号后清理资源并退出
 *
 * 注意：LVGL 非线程安全，所有 LVGL API 调用都在此线程内完成。
 */
void DemoApp::lv_loop_task() {
    LOG_D("LV Task Start!!");
    lv_init();
    lv_tick_set_cb(lv_tick_get);
    lv_log_register_print_cb(lv_log_print);

    // 创建显示对象，设置用户数据为 this 以便在静态回调中获取实例
    auto *disp = lv_display_create(app_width, app_height);
    lv_display_set_user_data(disp, this);
    lv_display_set_flush_cb(disp, DemoApp::lv_flush_cb_static);

    // 分配绘制缓冲区（PARTIAL 模式），大小为 1 倍屏幕像素数（RGB565 = 2 字节/像素）
    // 之前使用 10 倍缓冲约 1.5MB，优化后仅需约 300KB，显著降低内存占用
    size_t buf_size = (size_t) app_width * app_height * 2;
    auto *buf = malloc(buf_size);
    lv_display_set_buffers(disp, buf, nullptr, buf_size, LV_DISPLAY_RENDER_MODE_PARTIAL);

    // 创建触摸输入设备（指针类型）
    auto *indev = lv_indev_create();
    lv_indev_set_user_data(indev, this);
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, DemoApp::lv_touch_cb_static);

    //--- app ---
    lv_demo_widgets();
    //--- app ---

    // 首帧前先提交一次空帧，确保 NativeWindow 缓冲区已初始化
    if (ANativeWindow_lock(window, &windowBuffer, nullptr) == 0) {
        ANativeWindow_unlockAndPost(window);
    }

    // LVGL 主循环：lv_timer_handler 返回下次需要运行的时间（毫秒），
    // 据此动态调整休眠时长，避免不必要的 CPU 唤醒
    while (bIsRunning) {
        uint32_t time_till_next = lv_timer_handler();
        // 限制休眠范围：最少 1ms 避免忙等，最多 33ms（约 30fps）
        if (time_till_next < 1) time_till_next = 1;
        if (time_till_next > 33) time_till_next = 33;
        usleep(time_till_next * 1000);
    }

    // 清理资源
    ANativeWindow_release(window);
    lv_deinit();
    free(buf);
    if (surface_buf != nullptr) {
        free(surface_buf);
        surface_buf = nullptr;
    }
    window = nullptr;
    LOG_D("LV App Stopped!!");
}

/**
 * 停止渲染线程。设置运行标志为 false 并等待线程退出。
 * 线程退出后会自动 release NativeWindow 引用。
 */
void DemoApp::stop() {
    bIsRunning = false;
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

/**
 * 处理来自 Java 层的触摸事件。
 * 将屏幕物理坐标按比例映射为 LVGL 逻辑坐标后存储，
 * 供 lv_touch_callback 在下次读取时使用。
 *
 * @param touch 1=按下，0=释放
 * @param x     屏幕物理 X 坐标
 * @param y     屏幕物理 Y 坐标
 */
void DemoApp::onTouch(int touch, int x, int y) {
    if (screen_width <= 0 || screen_height <= 0) {
        return;
    }
    isTouch = touch;
    // 坐标映射：屏幕物理坐标 → LVGL 逻辑坐标
    touchX = x * app_width / screen_width;
    touchY = y * app_height / screen_height;
}

// 静态刷新回调包装：通过 user_data 获取实例后调用实例方法
void DemoApp::lv_flush_cb_static(lv_display_t *display, const lv_area_t *area, uint8_t *px_map) {
    auto *app = (DemoApp *) lv_display_get_user_data(display);
    app->lv_flush_callback(display, area, px_map);
}

// 静态触摸回调包装：通过 user_data 获取实例后调用实例方法
void DemoApp::lv_touch_cb_static(lv_indev_t *indev_drv, lv_indev_data_t *data) {
    auto *app = (DemoApp *) lv_indev_get_user_data(indev_drv);
    app->lv_touch_callback(indev_drv, data);
}

/**
 * 析构函数，确保渲染线程已停止并释放所有资源。
 */
DemoApp::~DemoApp() {
    stop();
    LOG_I("LVApp::~LVApp()!!");
}

// ===================== JNI 桥接层 =====================
// 连接 Kotlin 层（LVGLDemoView 的 external fun）与 Native 引擎。
// 全局唯一 DemoApp 实例，整个进程共用一个 LVGL 引擎。

static DemoApp *GlobalApp = nullptr;

/** 创建 LVApp 实例（若尚未创建） */
static void nativeCreate(
        JNIEnv *env,
        jobject /* this */
) {
    if (GlobalApp == nullptr) {
        GlobalApp = new DemoApp();
    }
}

/** 启动 LVGL 渲染，将 Java Surface 转换为 ANativeWindow 后传给 LVApp */
static void nativeStart(
        JNIEnv *env,
        jobject /* this */,
        jobject surface
) {
    if (GlobalApp == nullptr || surface == nullptr) {
        return;
    }
    ANativeWindow *window = ANativeWindow_fromSurface(env, surface);
    if (window == nullptr) {
        return;
    }
    GlobalApp->start(window);
}

/** 转发触摸事件到 Native 层 */
static void nativeOnTouch(
        JNIEnv *env,
        jobject /* this */,
        jint touch,
        jint x,
        jint y
) {
    if (GlobalApp == nullptr) {
        return;
    }

    GlobalApp->onTouch(touch, x, y);
}

/** 停止渲染线程（暂停渲染，不销毁实例） */
static void nativeStop(
        JNIEnv *env,
        jobject /* this */
) {
    if (GlobalApp == nullptr) {
        return;
    }
    GlobalApp->stop();
}

/** 销毁 LVApp 实例，释放所有 Native 资源 */
static void nativeDestroy(
        JNIEnv *env,
        jobject /* this */
) {
    if (GlobalApp != nullptr) {
        delete GlobalApp;
        GlobalApp = nullptr;
    }
}

static const JNINativeMethod gMethods[] = {
        {"lvglCreate", "()V", (void *) nativeCreate},
        {"lvglStart", "(Landroid/view/Surface;)V", (void *) nativeStart},
        {"lvglOnTouch", "(III)V", (void *) nativeOnTouch},
        {"lvglStop", "()V", (void *) nativeStop},
        {"lvglDestroy", "()V", (void *) nativeDestroy},
};

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    LOG_D("JNI_OnLoad()");
    JNIEnv *env = nullptr;
    if (vm->GetEnv((void **) &env, JNI_VERSION_1_4) != JNI_OK) {
        return -1;
    }
    jclass clazz = env->FindClass("com/wyq0918dev/lvgl/demo/ui/view/LVGLDemoView");
    if (clazz == nullptr) {
        LOG_E("JNI_OnLoad: cannot find class.");
        return -1;
    }
    jint count = sizeof(gMethods) / sizeof(gMethods[0]);
    if (env->RegisterNatives(clazz, gMethods, count) != JNI_OK) {
        LOG_E("JNI_OnLoad: RegisterNatives failed");
        return -1;
    }
    return JNI_VERSION_1_4;
}
