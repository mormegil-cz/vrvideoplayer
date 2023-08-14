#ifndef VRVIDEOPLAYER_RENDERER_H
#define VRVIDEOPLAYER_RENDERER_H

#include <array>

#include <EGL/egl.h>
#include <GLES/gl.h>

#include <cardboard.h>

#include "glm/mat4x4.hpp"

#include "TexturedMesh.h"
#include "GLUtils.h"

/**
 * Is the input video monoscopic or stereoscopic, and if stereoscopic, how are the views stored?
 */
enum class InputVideoLayout {
    MONO = 1,
    STEREO_HORIZ = 2,
    STEREO_VERT = 3,
    ANAGLYPH_RED_CYAN = 4,
};

/**
 * What is the geometry of the input video?
 */
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

/**
 * How we should render the output?
 */
enum class OutputMode {
    MONO_LEFT = 1,
    MONO_RIGHT = 2,
    CARDBOARD_STEREO = 3,
};

class Renderer {
public:
    Renderer(JavaVM *vm, jobject javaContextObj, jobject javaAssetMgrObj,
             jobject javaVideoTexturePlayerObj);

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
    JavaVM *javaVm;
    jobject javaContext;
    jobject javaAssetMgr;
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
    GLuint programVideo;
    GLint programVideoParamPosition;
    GLint programVideoParamUV;
    GLint programVideoParamMVPMatrix;
    GLint programVideoParamColorMapMatrix;
    GLuint programVRGui;
    GLint programVRGuiParamPosition;
    GLint programVRGuiParamUV;
    GLint programVRGuiParamMVPMatrix;
    GLuint program2D;
    GLint program2DParamPosition;

    GLuint videoTexture;
    GLuint renderTexture;
    GLuint buttonTexture;

    GLuint framebuffer;

    std::array<glm::mat4, 2> cardboardEyeMatrices;
    std::array<glm::mat4, 2> cardboardProjectionMatrices;
    std::array<CardboardEyeTextureDescription, 2> cardboardEyeTextureDescriptions;

    std::array<TexturedMesh, 2> eyeMeshes;

    glm::mat4 viewMatrix;
    glm::vec3 viewEulerAngles;

    bool vrGuiShown = false;
    bool isHeadGesturingUp = false;
    float vrGuiCenterTheta = 0.0f;

    bool UpdateDeviceParams();

    void GlSetup();

    void GlTeardown();

    void ComputeMesh();

    void UpdatePose();

    void RenderPointer();

    void RenderCardboardAlignLine();

    glm::mat4 BuildMVPMatrix(int eye);
    glm::mat4 BuildColorMapMatrix(int eye);
};

#endif //VRVIDEOPLAYER_RENDERER_H
