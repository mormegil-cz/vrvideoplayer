#ifndef VRVIDEOPLAYER_RENDERER_H
#define VRVIDEOPLAYER_RENDERER_H

#include <array>

#include <EGL/egl.h>
#include <GLES/gl.h>

#include <cardboard.h>

#include "glm/mat4x4.hpp"

#include "TexturedMesh.h"
#include "GLUtils.h"

enum class InputVideoLayout {
    MONO = 1,
    STEREO_HORIZ = 2,
    STEREO_VERT = 3,
};

enum class InputVideoMode {
    PLAIN_FOV = 1,
    EQUIRECT_180 = 2,
    EQUIRECT_360 = 3,
    CUBE_MAP = 4,
    EQUIANG_CUBE_MAP = 5,
    PYRAMID = 6,
    PANORAMA_180 = 7,
    PANORAMA_360 = 8,
};

enum class OutputMode {
    MONO_LEFT = 1,
    MONO_RIGHT = 2,
    CARDBOARD_STEREO = 3,
};

class Renderer {
public:
    Renderer(JavaVM *vm, jobject javaContextObj, jobject javaVideoTexturePlayerObj);

    virtual ~Renderer();

    void OnSurfaceCreated(JNIEnv *env);

    void SetOptions(InputVideoLayout layout, InputVideoMode inputMode,
                    OutputMode outputMode);

    void ScanCardboardQr();

    void SetScreenParams(int width, int height);

    void DrawFrame();

    void OnPause();

    void OnResume();

private:
    jobject javaContext;
    jobject javaVideoTexturePlayer;

    CardboardHeadTrackerPointer cardboardHeadTracker;
    CardboardLensDistortionPointer cardboardLensDistortion;
    CardboardDistortionRendererPointer cardboardDistortionRenderer;

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
    GLuint renderTexture;

    GLuint framebuffer;

    std::array<TexturedMesh, 2> eyeMeshes;

    glm::mat4 viewMatrix;
    std::array<glm::mat4, 2> cardboardEyeMatrices;
    std::array<glm::mat4, 2> cardboardProjectionMatrices;
    std::array<CardboardEyeTextureDescription, 2> cardboardEyeTextureDescriptions;

    bool UpdateDeviceParams();

    void GlSetup();

    void GlTeardown();

    void ComputeMesh();

    void UpdatePose();

    glm::mat4 BuildMVPMatrix(int eye);
};

#endif //VRVIDEOPLAYER_RENDERER_H
