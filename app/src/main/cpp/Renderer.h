#ifndef VRVIDEOPLAYER_RENDERER_H
#define VRVIDEOPLAYER_RENDERER_H

#include <pthread.h>
#include <EGL/egl.h>
#include <GLES/gl.h>

#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

#include "glm/mat4x4.hpp"

class Renderer {

public:
    Renderer(JavaVM* vm, jobject obj, jobject javaAssetMgrObj, jobject javaVideoTexturePlayerObj);
    virtual ~Renderer();

    void OnSurfaceCreated(JNIEnv* env);
    void SetScreenParams(int width, int height);
    void DrawFrame();
    void OnPause();
    void OnResume();

private:
    jobject javaAssetMgr;
    jobject javaVideoTexturePlayer;

    bool screenParamsChanged;
    bool deviceParamsChanged;
    int screenWidth;
    int screenHeight;

    bool glInitialized;

    unsigned long frameCount;
    GLfloat angle;

    GLuint obj_program;
    GLint obj_position_param;
    GLint obj_uv_param;
    GLint obj_color_param;
    GLint obj_modelview_projection_param;

    GLuint depthRenderBuffer;
    GLuint cubeTexture;

    bool UpdateDeviceParams();
    void GlSetup();
    void GlTeardown();

    glm::mat4 BuildMVPMatrix();
};

#endif //VRVIDEOPLAYER_RENDERER_H
