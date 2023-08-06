#include <jni.h>
#include <android/log.h>

#include <android/native_window.h>
#include <android/native_window_jni.h>

#include "logger.h"
#include "Renderer.h"

#define LOG_TAG "VRVideoPlayerN"

inline jlong toJava(Renderer* native_app) {
    return reinterpret_cast<intptr_t>(native_app);
}

inline Renderer* fromJava(jlong ptr) {
    return reinterpret_cast<Renderer*>(ptr);
}

static JavaVM* javaVm;

extern "C" JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* /*reserved*/) {
    javaVm = vm;
    return JNI_VERSION_1_6;
}

extern "C" JNIEXPORT jlong JNICALL
Java_cz_mormegil_vrvideoplayer_NativeLibrary_nativeInit(
        JNIEnv* /* env */,
        jobject obj,
        jobject assets) {
    LOG_DEBUG("nativeOnStart");
    return toJava(new Renderer(javaVm, obj, assets, obj));
}

extern "C" JNIEXPORT void JNICALL
Java_cz_mormegil_vrvideoplayer_NativeLibrary_nativeOnResume(
        JNIEnv* /* env */,
        jobject /* this */,
        jlong native_app) {
    LOG_DEBUG("nativeOnResume");
    fromJava(native_app)->OnResume();
}

extern "C" JNIEXPORT void JNICALL
Java_cz_mormegil_vrvideoplayer_NativeLibrary_nativeOnPause(
        JNIEnv* /* env */,
        jobject /* this */,
        jlong native_app) {
    LOG_DEBUG("nativeOnPause");
    fromJava(native_app)->OnPause();
}

extern "C" JNIEXPORT void JNICALL
Java_cz_mormegil_vrvideoplayer_NativeLibrary_nativeOnDestroy(
        JNIEnv* /* env */,
        jobject /* this */,
        jlong native_app) {
    LOG_DEBUG("nativeOnDestroy");
    delete fromJava(native_app);
}

extern "C" JNIEXPORT void JNICALL
Java_cz_mormegil_vrvideoplayer_NativeLibrary_nativeOnSurfaceCreated(
        JNIEnv* env,
        jobject /* this */,
        jlong native_app) {
    LOG_DEBUG("nativeOnSurfaceCreated");
    fromJava(native_app)->OnSurfaceCreated(env);
}

extern "C" JNIEXPORT void JNICALL
Java_cz_mormegil_vrvideoplayer_NativeLibrary_nativeSetScreenParams(
        JNIEnv* /* env */,
        jobject /* this */,
        jlong native_app,
        jint width,
        jint height) {
    LOG_DEBUG("nativeSetScreenParams");
    fromJava(native_app)->SetScreenParams(width, height);
}

extern "C" JNIEXPORT void JNICALL
Java_cz_mormegil_vrvideoplayer_NativeLibrary_nativeDrawFrame(
        JNIEnv* /* jenv */,
        jobject /* this */,
        jlong native_app) {
    // LOG_DEBUG("nativeDrawFrame");
    fromJava(native_app)->DrawFrame();
}
