#ifndef VRVIDEOPLAYER_RENDERER_H
#define VRVIDEOPLAYER_RENDERER_H

#include <pthread.h>
#include <EGL/egl.h>
#include <GLES/gl.h>

#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

class Renderer {

public:
    Renderer(JavaVM* vm, jobject obj, jobject asset_mgr_obj, jobject java_app_obj);
    virtual ~Renderer();

    void OnSurfaceCreated(JNIEnv* env);
    void SetScreenParams(int width, int height);
    void DrawFrame();
    void OnPause();
    void OnResume();

private:

    jobject java_asset_mgr;
    jobject java_app_obj;
    AAssetManager* asset_mgr;

    bool screen_params_changed;
    bool device_params_changed;
    int screen_width;
    int screen_height;

    GLfloat angle;
    GLuint obj_program;
    GLint obj_position_param;
    GLint obj_uv_param;
    GLint obj_color_param;
    GLint obj_modelview_projection_param;

    bool UpdateDeviceParams();

    void GlSetup();

    void GlTeardown();
};

#endif //VRVIDEOPLAYER_RENDERER_H
