#include "DemoApp.hpp"
#include <android/native_window_jni.h>
#include <jni.h>
#include <demos/lv_demos.h>
#include <unistd.h>

using namespace std;

// PARTIAL 模式下 LVGL 分块回调；累积整帧后整体提交，避免多缓冲局部刷新导致闪烁
void DemoApp::LvFlushCallback(lv_display_t *Display, const lv_area_t *Area, uint8_t *PxMap) {
    if (bIsRunning && Window != nullptr) {
        if (SurfaceBuffer == nullptr) {
            SurfaceSize = sizeof(uint32_t) * WindowBuffer.stride * WindowBuffer.height;
            SurfaceBuffer = (uint32_t *) malloc(SurfaceSize);
        }
        int Width = Area->x2 - Area->x1 + 1;
        int Height = Area->y2 - Area->y1 + 1;
        auto *Src = (uint16_t *) PxMap;
        auto Stride = WindowBuffer.stride;
        // 逐像素 RGB565 → RGBA_8888
        for (int I = 0; I < Height; I++) {
            auto *Dst = &SurfaceBuffer[(Area->y1 + I) * Stride + Area->x1];
            auto *SrcLine = &Src[I * Width];
            for (int J = 0; J < Width; J++) {
                uint16_t Color = SrcLine[J];
                auto Red = (uint8_t) (((Color >> 11) & 0x1F) * 255 / 31);
                auto Green = (uint8_t) (((Color >> 5) & 0x3F) * 255 / 63);
                auto Blue = (uint8_t) ((Color & 0x1F) * 255 / 31);
                Dst[J] = (0xFFu << 24) | ((uint32_t) Blue << 16) | ((uint32_t) Green << 8) | Red;
            }
        }
        if (ANativeWindow_lock(Window, &WindowBuffer, nullptr) == 0) {
            memcpy(WindowBuffer.bits, SurfaceBuffer, SurfaceSize);
            ANativeWindow_unlockAndPost(Window);
        }
    }
    lv_disp_flush_ready(Display);
}

// 触摸回调：有触摸则上报坐标与按下状态，否则上报释放
void DemoApp::LvTouchCallback(lv_indev_t *IndevDriver, lv_indev_data_t *Data) {
    if (bIsTouch) {
        Data->point.x = (short) TouchX;
        Data->point.y = (short) TouchY;
        Data->state = LV_INDEV_STATE_PR;
    } else {
        Data->state = LV_INDEV_STATE_REL;
    }
}

// 按级别转发 LVGL 日志到 logcat
void DemoApp::LvLogPrint(lv_log_level_t Level, const char *Buf) {
    switch (Level) {
        case LV_LOG_LEVEL_INFO:
            LOG_I("%s", Buf);
            break;
        case LV_LOG_LEVEL_WARN:
            LOG_W("%s", Buf);
            break;
        case LV_LOG_LEVEL_ERROR:
            LOG_E("%s", Buf);
            break;
        case LV_LOG_LEVEL_TRACE:
            LOG_D("%s", Buf);
            break;
        default:
            LOG_I("%s", Buf);
    }
}

// 心跳源：毫秒时间戳
uint32_t DemoApp::LvTickGet() {
    static struct timeval TimeVal;
    gettimeofday(&TimeVal, nullptr);
    return (TimeVal.tv_sec * 1000) + (TimeVal.tv_usec / 1000);
}

// 启动渲染：先停旧线程，设置窗口几何（RGBA_8888），再起新线程
void DemoApp::StartRender(ANativeWindow *Win) {
    StopRender();
    if (Win == nullptr) {
        return;
    }
    Window = Win;
    ScreenWidth = ANativeWindow_getWidth(Window);
    ScreenHeight = ANativeWindow_getHeight(Window);
    if (ANativeWindow_setBuffersGeometry(Window, AppWidth, AppHeight, WINDOW_FORMAT_RGBA_8888) != 0) {
        LOG_E("Failed to set NativeWindow geometry [%d x %d]", AppWidth, AppHeight);
        ANativeWindow_release(Window);
        Window = nullptr;
        return;
    }
    LOG_D("LV Screen [%d x %d]", AppWidth, AppHeight);
    bIsRunning = true;
    RenderThread = thread(&DemoApp::LvLoopTask, this);
}

// 渲染主循环：初始化 LVGL，建显示/输入，跑 lv_timer_handler 直至停止
void DemoApp::LvLoopTask() {
    LOG_D("LV Task Start!!");
    lv_init();
    lv_tick_set_cb(LvTickGet);
    lv_log_register_print_cb(LvLogPrint);

    auto *Disp = lv_display_create(AppWidth, AppHeight);
    lv_display_set_user_data(Disp, this);
    lv_display_set_flush_cb(Disp, DemoApp::LvFlushCbStatic);

    size_t BufSize = (size_t) AppWidth * AppHeight * 2;
    auto *Buf = malloc(BufSize);
    lv_display_set_buffers(Disp, Buf, nullptr, BufSize, LV_DISPLAY_RENDER_MODE_PARTIAL);

    auto *Indev = lv_indev_create();
    lv_indev_set_user_data(Indev, this);
    lv_indev_set_type(Indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(Indev, DemoApp::LvTouchCbStatic);

    lv_demo_widgets();

    // 首帧前提交一次空帧，初始化窗口缓冲
    if (ANativeWindow_lock(Window, &WindowBuffer, nullptr) == 0) {
        ANativeWindow_unlockAndPost(Window);
    }

    // 主循环：按 LVGL 返回间隔休眠（1~33ms，约 30fps）
    while (bIsRunning) {
        uint32_t TimeTillNext = lv_timer_handler();
        if (TimeTillNext < 1) TimeTillNext = 1;
        if (TimeTillNext > 33) TimeTillNext = 33;
        usleep(TimeTillNext * 1000);
    }

    ANativeWindow_release(Window);
    lv_deinit();
    free(Buf);
    if (SurfaceBuffer != nullptr) {
        free(SurfaceBuffer);
        SurfaceBuffer = nullptr;
    }
    Window = nullptr;
    LOG_D("LV App Stopped!!");
}

// 停止：置标志并 join 等待线程退出
void DemoApp::StopRender() {
    bIsRunning = false;
    if (RenderThread.joinable()) {
        RenderThread.join();
    }
}

// 触摸入口：保存状态并按比例映射坐标
void DemoApp::OnTouch(bool Touch, int X, int Y) {
    if (ScreenWidth <= 0 || ScreenHeight <= 0) {
        return;
    }
    bIsTouch = Touch;
    TouchX = X * AppWidth / ScreenWidth;
    TouchY = Y * AppHeight / ScreenHeight;
}

// 静态刷新回调 → 实例方法
void DemoApp::LvFlushCbStatic(lv_display_t *Display, const lv_area_t *Area, uint8_t *PxMap) {
    auto *App = (DemoApp *) lv_display_get_user_data(Display);
    App->LvFlushCallback(Display, Area, PxMap);
}

// 静态触摸回调 → 实例方法
void DemoApp::LvTouchCbStatic(lv_indev_t *IndevDriver, lv_indev_data_t *Data) {
    auto *App = (DemoApp *) lv_indev_get_user_data(IndevDriver);
    App->LvTouchCallback(IndevDriver, Data);
}

DemoApp::~DemoApp() {
    StopRender();
    LOG_I("LVApp::~LVApp()!!");
}

static DemoApp *GlobalApp = nullptr;

// 创建单例
static void NativeCreate(JNIEnv *Env, jobject) {
    if (GlobalApp == nullptr) {
        GlobalApp = new DemoApp();
    }
}

// Surface → ANativeWindow → 启动渲染
static void NativeStart(JNIEnv *Env, jobject, jobject Surface) {
    if (GlobalApp == nullptr || Surface == nullptr) {
        return;
    }
    ANativeWindow *NativeWindow = ANativeWindow_fromSurface(Env, Surface);
    if (NativeWindow == nullptr) {
        return;
    }
    GlobalApp->StartRender(NativeWindow);
}

// 转发触摸
static void NativeOnTouch(JNIEnv *Env, jobject, jboolean Touch, jint X, jint Y) {
    if (GlobalApp == nullptr) {
        return;
    }
    GlobalApp->OnTouch(Touch, X, Y);
}

// 暂停渲染
static void NativeStop(JNIEnv *Env, jobject) {
    if (GlobalApp == nullptr) {
        return;
    }
    GlobalApp->StopRender();
}

// 销毁单例
static void NativeDestroy(JNIEnv *Env, jobject) {
    if (GlobalApp != nullptr) {
        delete GlobalApp;
        GlobalApp = nullptr;
    }
}

static const JNINativeMethod NativeMethods[] = {
        {"lvglCreate", "()V", (void *) NativeCreate},
        {"lvglStart", "(Landroid/view/Surface;)V", (void *) NativeStart},
        {"lvglOnTouch", "(ZII)V", (void *) NativeOnTouch},
        {"lvglStop", "()V", (void *) NativeStop},
        {"lvglDestroy", "()V", (void *) NativeDestroy},
};

// 注册 JNI 方法
extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *Vm, void *Reserved) {
    LOG_D("JNI_OnLoad()");
    JNIEnv *Env = nullptr;
    if (Vm->GetEnv((void **) &Env, JNI_VERSION_1_6) != JNI_OK) {
        return -1;
    }
    jclass Clazz = Env->FindClass("com/wyq0918dev/lvgl/demo/ui/view/LVGLDemoView");
    if (Clazz == nullptr) {
        LOG_E("JNI_OnLoad: cannot find class.");
        return -1;
    }
    jint Count = sizeof(NativeMethods) / sizeof(NativeMethods[0]);
    if (Env->RegisterNatives(Clazz, NativeMethods, Count) != JNI_OK) {
        LOG_E("JNI_OnLoad: RegisterNatives failed");
        return -1;
    }
    return JNI_VERSION_1_6;
}
