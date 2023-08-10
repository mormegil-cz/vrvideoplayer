#ifndef VRVIDEOPLAYER_RENDERER_H
#define VRVIDEOPLAYER_RENDERER_H

#include <array>

#include <EGL/egl.h>
#include <GLES/gl.h>

// TODO: Remove asset manager?
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

#include <cardboard.h>

#include "glm/mat4x4.hpp"

#include "TexturedMesh.h"
#include "GLUtils.h"

enum class InputVideoLayout {
    MONO,
    STEREO_HORIZ,
    STEREO_VERT,
};

enum class InputVideoMode {
    PLAIN_FOV,
    EQUIRECT_180,
    EQUIRECT_360,
    CUBE_MAP,
    EQUIANG_CUBE_MAP,
    PYRAMID,
    PANORAMA_180,
    PANORAMA_360,
};

enum class OutputMode {
    MONO_LEFT,
    MONO_RIGHT,
    CARDBOARD_STEREO,
};

class Renderer {
public:
    Renderer(JavaVM *vm, jobject obj, jobject javaAssetMgrObj, jobject javaVideoTexturePlayerObj);

    virtual ~Renderer();

    void OnSurfaceCreated(JNIEnv *env);

    void SetOptions(InputVideoLayout layout, InputVideoMode inputMode,
                    OutputMode outputMode);

    void SetScreenParams(int width, int height);

    void DrawFrame();

    void OnPause();

    void OnResume();

private:
    jobject javaContext;
    // TODO: Remove asset manager?
    jobject javaAssetMgr;
    jobject javaVideoTexturePlayer;

    CardboardHeadTrackerPointer cardboardHeadTracker;

    bool screenParamsChanged;
    bool deviceParamsChanged;
    int screenWidth;
    int screenHeight;

    bool glInitialized;

    InputVideoLayout inputVideoLayout;
    InputVideoMode inputVideoMode;
    OutputMode outputMode;

    unsigned long frameCount;

    GLuint program;
    GLint programParamPosition;
    GLint programParamUV;
    GLint programParamMVPMatrix;

    GLuint videoTexture;

    std::array<TexturedMesh, 2> eyeMeshes;

    bool UpdateDeviceParams();

    void GlSetup();

    void GlTeardown();

    void ComputeMesh();

    glm::mat4 BuildMVPMatrix(int eye);
};

#endif //VRVIDEOPLAYER_RENDERER_H
