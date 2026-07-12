#include <iostream>
#include <thread>
#include <lvgl.h>
#include <lv_demos.h>
#include <jni.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include "DemoApp.hpp"
#include "logging.hpp"

using namespace std;

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
    JNIEnv *env = nullptr;
    if (vm->GetEnv((void **) &env, JNI_VERSION_1_4) != JNI_OK) {
        return -1;
    }
    jclass clazz = env->FindClass("com/wyq0918dev/lvgl/demo/ui/view/LVGLDemoView");
    if (clazz == nullptr) {
        LOG_E("JNI_OnLoad: cannot find class com/hzy/lvgl/demo/LVGLView");
        return -1;
    }
    jint count = sizeof(gMethods) / sizeof(gMethods[0]);
    if (env->RegisterNatives(clazz, gMethods, count) != JNI_OK) {
        LOG_E("JNI_OnLoad: RegisterNatives failed");
        return -1;
    }
    LOG_D("JNI_OnLoad()");
    return JNI_VERSION_1_4;
}