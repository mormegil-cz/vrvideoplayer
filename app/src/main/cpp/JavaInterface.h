#ifndef VR_VIDEO_PLAYER_JAVAINTERFACE_H
#define VR_VIDEO_PLAYER_JAVAINTERFACE_H

#include <string>

#include <jni.h>
#include <GLES/gl.h>

#include "VRGuiButton.h"

class JavaInterface {
public:
    JavaInterface(JavaVM *vm, jobject javaContextObj, jobject javaAssetMgrObj,
                  jobject javaVideoTexturePlayerObj);

    ~JavaInterface();

    bool InitializePlayback(JNIEnv *env, GLuint textureName);

    bool LoadPngFromAssetManager(JNIEnv *env, int target,
                                 const std::string &path);

    bool ExecuteButtonAction(JNIEnv *env, const ButtonAction action);

private:
    JavaVM *javaVm;
    jobject javaContext;
    jobject javaAssetMgr;
    jobject javaVideoTexturePlayer;

    jclass javaClassBitmapFactory;
    jclass javaClassGlUtils;

    jmethodID javaMethodVideoTexturePlayerInitializePlayback;
    jmethodID javaMethodBitmapFactoryDecodeStream;
    jmethodID javaMethodAssetManagerOpen;
    jmethodID javaMethodGlUtilsTexImage2D;
};

#endif //VR_VIDEO_PLAYER_JAVAINTERFACE_H
