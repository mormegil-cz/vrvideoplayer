#include <GLES/gl.h>
#include "JavaInterface.h"
#include "logger.h"

#define LOG_TAG "VRVideoPlayerJ"

class JavaLocalRef {
public:
    explicit JavaLocalRef(JNIEnv *env, jobject obj) : env(env), obj(obj) {}

    ~JavaLocalRef() {
        env->DeleteLocalRef(obj);
    }

private:
    JNIEnv *env;
    jobject obj;
};

template<typename E>
constexpr typename std::underlying_type<E>::type to_underlying(E e) noexcept {
    return static_cast<typename std::underlying_type<E>::type>(e);
}

JavaInterface::JavaInterface(JavaVM *vm, jobject javaContextObj, jobject javaAssetMgrObj,
                             jobject javaVideoTexturePlayerObj) :
        javaVm(vm) {

    JNIEnv *env;
    vm->GetEnv((void **) &env, JNI_VERSION_1_6);

    javaContext = env->NewGlobalRef(javaContextObj);
    javaAssetMgr = env->NewGlobalRef(javaAssetMgrObj);
    javaVideoTexturePlayer = env->NewGlobalRef(javaVideoTexturePlayerObj);


    jclass videoTexturePlayerClass = env->FindClass("cz/mormegil/vrvideoplayer/VideoTexturePlayer");
    javaMethodVideoTexturePlayerInitializePlayback = env->GetMethodID(videoTexturePlayerClass,
                                                                      "initializePlayback",
                                                                      "(I)V");

    jclass bitmapFactoryClass =
            env->FindClass("android/graphics/BitmapFactory");
    jclass assetManagerClass =
            env->FindClass("android/content/res/AssetManager");

    jclass glUtilsClass = env->FindClass("android/opengl/GLUtils");

    javaClassBitmapFactory = reinterpret_cast<jclass>(env->NewGlobalRef(bitmapFactoryClass));
    javaClassGlUtils = reinterpret_cast<jclass>(env->NewGlobalRef(glUtilsClass));

    javaMethodBitmapFactoryDecodeStream = env->GetStaticMethodID(
            bitmapFactoryClass,
            "decodeStream",
            "(Ljava/io/InputStream;)Landroid/graphics/Bitmap;");
    javaMethodAssetManagerOpen = env->GetMethodID(
            assetManagerClass,
            "open",
            "(Ljava/lang/String;)Ljava/io/InputStream;");
    javaMethodGlUtilsTexImage2D = env->GetStaticMethodID(
            glUtilsClass,
            "texImage2D",
            "(IILandroid/graphics/Bitmap;I)V");
}

JavaInterface::~JavaInterface() {
    JNIEnv *env;
    javaVm->GetEnv((void **) &env, JNI_VERSION_1_6);

    env->DeleteGlobalRef(javaClassBitmapFactory);

    env->DeleteGlobalRef(javaVideoTexturePlayer);
    env->DeleteGlobalRef(javaAssetMgr);
    env->DeleteGlobalRef(javaContext);
}

bool JavaInterface::InitializePlayback(JNIEnv *env, GLuint textureName) {
    if (textureName > INT32_MAX) {
        // ??!?
        LOG_ERROR("Invalid texture name");
        return false;
    }
    jint textureNameJava = static_cast<jint>(textureName);

    env->CallVoidMethod(javaVideoTexturePlayer, javaMethodVideoTexturePlayerInitializePlayback,
                        textureNameJava);

    if (env->ExceptionOccurred() != nullptr) {
        LOG_ERROR("Java exception in initializePlayback");
        env->ExceptionClear();
        return false;
    }

    return true;
}

bool JavaInterface::LoadPngFromAssetManager(JNIEnv *env, int target, const std::string &path) {
    jstring javaPath = env->NewStringUTF(path.c_str());
    JavaLocalRef(env, javaPath);

    jobject imageStream =
            env->CallObjectMethod(javaAssetMgr, javaMethodAssetManagerOpen, javaPath);
    jobject imageObj = env->CallStaticObjectMethod(
            javaClassBitmapFactory, javaMethodBitmapFactoryDecodeStream, imageStream);
    if (env->ExceptionOccurred() != nullptr) {
        LOG_ERROR("Java exception while loading image");
        env->ExceptionClear();
        return false;
    }

    env->CallStaticVoidMethod(javaClassGlUtils, javaMethodGlUtilsTexImage2D, target, 0,
                              imageObj, 0);
    if (env->ExceptionOccurred() != nullptr) {
        LOG_ERROR("Java exception while setting image to texture");
        env->ExceptionClear();
        return false;
    }

    return true;
}

bool JavaInterface::ExecuteButtonAction(JNIEnv *env, const ButtonAction action) {
    int actionId = to_underlying(action);
    if (actionId > INT32_MAX) {
        // ??!?
        LOG_ERROR("Invalid button action");
        return false;
    }
    jint actionIdJava = static_cast<jint>(actionId);

    //env->CallVoidMethod(javaController, javaMethodControllerExecuteButtonAction, actionIdJava);

    if (env->ExceptionOccurred() != nullptr) {
        LOG_ERROR("Java exception in button action");
        env->ExceptionClear();
        return false;
    }

    return true;
}
