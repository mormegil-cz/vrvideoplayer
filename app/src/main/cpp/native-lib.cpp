#include <jni.h>
#include <android/log.h>

#include <android/native_window.h>
#include <android/native_window_jni.h>

#include "logger.h"
#include "Renderer.h"

#define LOG_TAG "VrVideoPlayerN"

static ANativeWindow *window = nullptr;
static Renderer *renderer = nullptr;

extern "C" JNIEXPORT void JNICALL
Java_cz_mormegil_vrvideoplayer_MainActivity_nativeOnStart(
        JNIEnv* /* jenv */,
        jobject /* this */) {
    LOG_INFO("nativeOnStart");
    renderer = new Renderer();
}

extern "C" JNIEXPORT void JNICALL
Java_cz_mormegil_vrvideoplayer_MainActivity_nativeOnResume(
        JNIEnv* /* jenv */,
        jobject /* this */) {
    LOG_INFO("nativeOnResume");
    renderer->start();
}

extern "C" JNIEXPORT void JNICALL
Java_cz_mormegil_vrvideoplayer_MainActivity_nativeOnPause(
        JNIEnv* /* jenv */,
        jobject /* this */) {
    LOG_INFO("nativeOnPause");
    renderer->stop();
}

extern "C" JNIEXPORT void JNICALL
Java_cz_mormegil_vrvideoplayer_MainActivity_nativeOnStop(
        JNIEnv* env,
        jobject /* this */) {
    LOG_INFO("nativeOnStop");
    delete renderer;
    renderer = 0;
}

extern "C" JNIEXPORT void JNICALL
Java_cz_mormegil_vrvideoplayer_MainActivity_nativeSetSurface(
        JNIEnv* jenv,
        jobject /* this */,
        jobject surface) {
    if (surface != 0) {
        window = ANativeWindow_fromSurface(jenv, surface);
        LOG_INFO("Got window %p", window);
        renderer->setWindow(window);
    } else {
        LOG_INFO("Releasing window");
        ANativeWindow_release(window);
    }
}
