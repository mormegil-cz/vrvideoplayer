#include "Renderer.h"

#include <cmath>

#include <array>
#include <fstream>

#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/log.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <cardboard.h>

#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include "glm/mat4x4.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtx/matrix_operation.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/scalar_constants.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "VRGuiButton.h"
#include "GLUtils.h"
#include "logger.h"
#include "VRGuiProgressBar.h"

#define LOG_TAG "VRVideoPlayerR"

constexpr uint64_t kPredictionTimeWithoutVsyncNanos = 50'000'000UL;
constexpr float kzNear = 0.1f;
constexpr float kzFar = 2.0f;

constexpr const char *kVertexShader = R"glsl(#version 300 es
uniform mat4 u_MVP;
in vec4 a_Position;
in vec2 a_UV;
out vec2 v_UV;

void main() {
  v_UV = a_UV;
  gl_Position = u_MVP * a_Position;
})glsl";

constexpr const char *kFragmentShader = R"glsl(#version 300 es
#extension GL_OES_EGL_image_external : enable
#extension GL_OES_EGL_image_external_essl3 : enable
precision mediump float;

uniform samplerExternalOES u_Texture;
uniform mat4 u_ColorMap;
in vec2 v_UV;
out vec4 fragColor;

void main() {
  fragColor = u_ColorMap * texture(u_Texture, v_UV);
})glsl";

constexpr const char *kFragmentShaderVRGui = R"glsl(#version 300 es
precision mediump float;

uniform sampler2D u_Texture;
in vec2 v_UV;
out vec4 fragColor;

void main() {
  fragColor = texture(u_Texture, v_UV);
})glsl";

constexpr const char *kVertexShader2D = R"glsl(#version 300 es
in vec4 a_Position;

void main() {
  gl_Position = a_Position;
})glsl";

constexpr const char *kFragmentShader2D = R"glsl(#version 300 es
precision mediump float;

out vec4 fragColor;

void main() {
  fragColor = vec4(1.0);
})glsl";

static constexpr float M_TWO_PI = (float) M_PI * 2.0f;

static constexpr float PLAIN_FOV_Z = -1.0f;

static constexpr float VR_GUI_BUTTON_GRID = M_PI * 8 / 180.0f;
static constexpr float VR_GUI_BUTTON_SIZE = M_PI * 7 / 180.0f;
static constexpr float VR_GUI_BUTTON_PHI_0 = -0.5f * VR_GUI_BUTTON_GRID;
static constexpr float VR_GUI_DISTANCE = kzFar * 0.4f;

static constexpr float HEAD_GESTURE_PITCH_LIMIT = glm::radians(60.0f);
static constexpr float HEAD_GESTURE_PITCH_LIMIT_RETURN = glm::radians(45.0f);

static constexpr int PROGRESS_BAR_SHOW_TIME = 3;

static constexpr glm::vec3 Y_AXIS = {0.0f, 1.0f, 0.0f};
static constexpr glm::vec4 NEG_Z_AXIS = {0.0f, 0.0f, -1.0f, 1.0f};

static constexpr std::array<float, 3> pointerCoords = {0.0f, 0.0f, VR_GUI_DISTANCE};
static constexpr std::array<float, 6> cardboardAlignLineCoords = {0.0f, -0.2f, 0.5f, 0.0f, -1.0f,
                                                                  0.5f};
static constexpr GLubyte trivial2DData[] = {0, 1};

static std::array<VRGuiButton, 10> vrGuiButtons{
        VRGuiButton(M_PI - 1 * VR_GUI_BUTTON_GRID, VR_GUI_BUTTON_PHI_0 - 1 * VR_GUI_BUTTON_GRID,
                    VR_GUI_DISTANCE, VR_GUI_BUTTON_SIZE, 0, 0, ButtonAction::RECENTER_2D,
                    ButtonBehavior::DELAYED_TRIGGER, true),
        VRGuiButton(M_PI - 0 * VR_GUI_BUTTON_GRID, VR_GUI_BUTTON_PHI_0 - 0 * VR_GUI_BUTTON_GRID,
                    VR_GUI_DISTANCE, VR_GUI_BUTTON_SIZE, 256, 0, ButtonAction::RECENTER_YAW,
                    ButtonBehavior::DELAYED_TRIGGER, true),
        VRGuiButton(M_PI - 2 * VR_GUI_BUTTON_GRID, VR_GUI_BUTTON_PHI_0 - 0 * VR_GUI_BUTTON_GRID,
                    VR_GUI_DISTANCE, VR_GUI_BUTTON_SIZE, 512, 0, ButtonAction::VOLUME_DOWN,
                    ButtonBehavior::AUTO_REPEAT, true),
        VRGuiButton(M_PI - 1 * VR_GUI_BUTTON_GRID, VR_GUI_BUTTON_PHI_0 - 0 * VR_GUI_BUTTON_GRID,
                    VR_GUI_DISTANCE, VR_GUI_BUTTON_SIZE, 768, 0, ButtonAction::VOLUME_UP,
                    ButtonBehavior::AUTO_REPEAT, true),
        VRGuiButton(M_PI - 2 * VR_GUI_BUTTON_GRID, VR_GUI_BUTTON_PHI_0 - 1 * VR_GUI_BUTTON_GRID,
                    VR_GUI_DISTANCE, VR_GUI_BUTTON_SIZE, 0, 256, ButtonAction::OPEN_FILE,
                    ButtonBehavior::DELAYED_TRIGGER, true),
        VRGuiButton(M_PI + 1 * VR_GUI_BUTTON_GRID, VR_GUI_BUTTON_PHI_0 - 1 * VR_GUI_BUTTON_GRID,
                    VR_GUI_DISTANCE, VR_GUI_BUTTON_SIZE, 256, 256, ButtonAction::PLAY,
                    ButtonBehavior::DELAYED_TRIGGER, false),
        VRGuiButton(M_PI + 1 * VR_GUI_BUTTON_GRID, VR_GUI_BUTTON_PHI_0 - 1 * VR_GUI_BUTTON_GRID,
                    VR_GUI_DISTANCE, VR_GUI_BUTTON_SIZE, 512, 256, ButtonAction::BACK,
                    ButtonBehavior::DELAYED_TRIGGER, true),
        VRGuiButton(M_PI + 2 * VR_GUI_BUTTON_GRID, VR_GUI_BUTTON_PHI_0 - 1 * VR_GUI_BUTTON_GRID,
                    VR_GUI_DISTANCE, VR_GUI_BUTTON_SIZE, 768, 256, ButtonAction::FORWARD,
                    ButtonBehavior::AUTO_REPEAT, true),
        VRGuiButton(M_PI + 2 * VR_GUI_BUTTON_GRID, VR_GUI_BUTTON_PHI_0 - 0 * VR_GUI_BUTTON_GRID,
                    VR_GUI_DISTANCE, VR_GUI_BUTTON_SIZE, 0, 512, ButtonAction::REWIND,
                    ButtonBehavior::AUTO_REPEAT, true),
        VRGuiButton(M_PI + 1 * VR_GUI_BUTTON_GRID, VR_GUI_BUTTON_PHI_0 - 0 * VR_GUI_BUTTON_GRID,
                    VR_GUI_DISTANCE, VR_GUI_BUTTON_SIZE, 256, 512, ButtonAction::PAUSE,
                    ButtonBehavior::DELAYED_TRIGGER, true)
};

static VRGuiProgressBar vrGuiProgressBar(0, 3 * VR_GUI_BUTTON_GRID, 6 * VR_GUI_BUTTON_GRID,
                                         0.5f * VR_GUI_BUTTON_GRID);
static time_t vrGuiProgressBarHideAt;

Renderer::Renderer(JavaVM *vm, jobject javaContextObj, jobject javaAssetMgrObj,
                   jobject javaVideoTexturePlayerObj)
        : glInitialized(false),
          screenParamsChanged(false),
          deviceParamsChanged(false),
          frameCount(0),
          inputVideoMode{},
          inputVideoLayout{},
          outputMode{},
          eyeMeshes{},
          viewMatrix{},
          cardboardHeadTracker{},
          javaInterface(vm, javaContextObj, javaAssetMgrObj, javaVideoTexturePlayerObj) {
    LOG_DEBUG("Renderer instance created");

    Cardboard_initializeAndroid(vm, javaContextObj);
    cardboardHeadTracker = CardboardHeadTrackerPointer(CardboardHeadTracker_create());

    SetOptions(InputVideoLayout::MONO, InputVideoMode::PLAIN_FOV, OutputMode::MONO_LEFT);
}

Renderer::~Renderer() {
    LOG_DEBUG("Renderer instance destroyed");
}

void Renderer::OnPause() {
    LOG_DEBUG("OnPause after %lu frames", frameCount);

    CardboardHeadTracker_pause(cardboardHeadTracker.get());
}

void Renderer::OnResume() {
    LOG_DEBUG("OnResume");

    frameCount = 0;

    // Parameters may have changed.
    deviceParamsChanged = true;

    CardboardHeadTracker_resume(cardboardHeadTracker.get());
}

void Renderer::SetScreenParams(int width, int height) {
    LOG_DEBUG("SetScreenParams(%d, %d)", width, height);

    screenWidth = width;
    screenHeight = height;
    screenAspect = (float) width / (float) height;
    screenParamsChanged = true;
}

static void
initStaticTexture(JNIEnv *env, jobject java_asset_mgr, GLuint &textureId, const std::string &path) {
    glGenTextures(1, &textureId);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    if (!LoadPngFromAssetManager(env, java_asset_mgr, GL_TEXTURE_2D, path)) {
        LOG_ERROR("Couldn't load texture");
        return;
    }
    glGenerateMipmap(GL_TEXTURE_2D);

    CHECK_GL_ERROR("Texture load");
}

void Renderer::InitVideoTexture(JNIEnv *env, GLuint &textureId) {
    glGenTextures(1, &textureId);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, textureId);

    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if (!javaInterface.InitializePlayback(env, textureId)) {
        LOG_ERROR("Couldn't initialize video texture");
        return;
    }
    CHECK_GL_ERROR("Video texture init");
}

void Renderer::InitStaticTexture(JNIEnv *env, GLuint &textureId, const std::string &path) {
    glGenTextures(1, &textureId);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    if (!javaInterface.LoadPngFromAssetManager(env, GL_TEXTURE_2D, path)) {
        LOG_ERROR("Couldn't load texture");
        return;
    }
    glGenerateMipmap(GL_TEXTURE_2D);

    CHECK_GL_ERROR("Texture load");
}

void Renderer::OnSurfaceCreated(JNIEnv *env) {
    LOG_DEBUG("OnSurfaceCreated");

    const GLuint vertexShader = LoadGLShader(GL_VERTEX_SHADER, kVertexShader);
    const GLuint fragmentShader = LoadGLShader(GL_FRAGMENT_SHADER, kFragmentShader);

    programVideo = glCreateProgram();
    glAttachShader(programVideo, vertexShader);
    glAttachShader(programVideo, fragmentShader);
    glLinkProgram(programVideo);
    glUseProgram(programVideo);
    CHECK_GL_ERROR("Video program");

    programVideoParamPosition = glGetAttribLocation(programVideo, "a_Position");
    programVideoParamUV = glGetAttribLocation(programVideo, "a_UV");
    programVideoParamMVPMatrix = glGetUniformLocation(programVideo, "u_MVP");
    programVideoParamColorMapMatrix = glGetUniformLocation(programVideo, "u_ColorMap");
    CHECK_GL_ERROR("Video program params");

    InitVideoTexture(env, videoTexture);

    const GLuint vertexShaderVRGui = LoadGLShader(GL_VERTEX_SHADER, kVertexShader);
    const GLuint fragmentShaderVRGui = LoadGLShader(GL_FRAGMENT_SHADER, kFragmentShaderVRGui);

    programVRGui = glCreateProgram();
    glAttachShader(programVRGui, vertexShaderVRGui);
    glAttachShader(programVRGui, fragmentShaderVRGui);
    glLinkProgram(programVRGui);
    glUseProgram(programVRGui);
    CHECK_GL_ERROR("VR Gui program");

    programVRGuiParamPosition = glGetAttribLocation(programVRGui, "a_Position");
    programVRGuiParamUV = glGetAttribLocation(programVRGui, "a_UV");
    programVRGuiParamMVPMatrix = glGetUniformLocation(programVRGui, "u_MVP");
    CHECK_GL_ERROR("VR Gui program params");

    InitStaticTexture(env, buttonTexture, "buttons-texture.png");

    const GLuint vertexShader2D = LoadGLShader(GL_VERTEX_SHADER, kVertexShader2D);
    const GLuint fragmentShader2D = LoadGLShader(GL_FRAGMENT_SHADER, kFragmentShader2D);

    program2D = glCreateProgram();
    glAttachShader(program2D, vertexShader2D);
    glAttachShader(program2D, fragmentShader2D);
    glLinkProgram(program2D);
    glUseProgram(program2D);
    CHECK_GL_ERROR("2D program");

    program2DParamPosition = glGetAttribLocation(program2D, "a_Position");
    CHECK_GL_ERROR("2D program params");
}

void Renderer::DrawFrame(float videoPosition, JNIEnv *env) {
    if (!UpdateDeviceParams()) {
        return;
    }

    UpdatePose(env);

    int minEye, maxEye;
    GLsizei eyeWidth;
    switch (outputMode) {
        case OutputMode::MONO_LEFT:
            minEye = 0;
            maxEye = 0;
            eyeWidth = screenWidth;
            break;
        case OutputMode::MONO_RIGHT:
            minEye = 1;
            maxEye = 1;
            eyeWidth = screenWidth;
            break;
        case OutputMode::CARDBOARD_STEREO:
            minEye = 0;
            maxEye = 1;
            eyeWidth = screenWidth / 2;
            break;
        default:
            assert(false);
    }

    if (outputMode == OutputMode::CARDBOARD_STEREO) {
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    } else {
        glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
    }
    CHECK_GL_ERROR("Framebuffer");

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glDisable(GL_SCISSOR_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClear(GL_COLOR_BUFFER_BIT);
    CHECK_GL_ERROR("Params");

    time_t now = time(nullptr);
    if (vrProgressBarShown) {
        if (now >= vrGuiProgressBarHideAt) {
            LOG_DEBUG("Hiding progress bar");
            vrProgressBarShown = false;
        } else {
            vrGuiProgressBar.setProgress(videoPosition);
        }
    }

    for (int eye = minEye; eye <= maxEye; ++eye) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_EXTERNAL_OES, videoTexture);
        glUseProgram(programVideo);
        glViewport((eye - minEye) * eyeWidth, 0, eyeWidth, screenHeight);

        auto mvpMatrix = BuildMVPMatrix(eye);
        glUniformMatrix4fv(programVideoParamMVPMatrix, 1, GL_FALSE, glm::value_ptr(mvpMatrix));

        auto colorMapMatrix = BuildColorMapMatrix(eye);
        glUniformMatrix4fv(programVideoParamColorMapMatrix, 1, GL_FALSE,
                           glm::value_ptr(colorMapMatrix));

        eyeMeshes[eye].Render(programVideoParamPosition, programVideoParamUV);
        CHECK_GL_ERROR("Render progress bar");

        if (vrProgressBarShown) {
            glUseProgram(program2D);
            vrGuiProgressBar.render(program2DParamPosition);
            CHECK_GL_ERROR("Render progress bar");
        }

        if (vrGuiShown) {
            glUseProgram(programVRGui);
            glBindTexture(GL_TEXTURE_2D, buttonTexture);
            glm::mat4 guiMvpMatrix = glm::rotate(mvpMatrix, (float) M_PI - vrGuiCenterTheta,
                                                 Y_AXIS);
            glUniformMatrix4fv(programVRGuiParamMVPMatrix, 1, GL_FALSE,
                               glm::value_ptr(guiMvpMatrix));
            for (const VRGuiButton &button: vrGuiButtons) {
                button.render(programVRGuiParamPosition, programVRGuiParamUV);
            }

            glUseProgram(program2D);
            RenderPointer();
            CHECK_GL_ERROR("Render GUI");
        }
    }

    if (outputMode == OutputMode::CARDBOARD_STEREO) {
        CardboardDistortionRenderer_renderEyeToDisplay(
                cardboardDistortionRenderer.get(), 0,
                0, 0, screenWidth, screenHeight,
                &cardboardEyeTextureDescriptions[0], &cardboardEyeTextureDescriptions[1]
        );
        CHECK_GL_ERROR("Render cardboard");

        glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
        glViewport(0, 0, screenWidth, screenHeight);
        glUseProgram(program2D);
        RenderCardboardAlignLine();
        CHECK_GL_ERROR("Align line");
    }

    ++frameCount;
}

void Renderer::RenderPointer() {
    // TODO: Pointer size? gl_PointSize?
    glEnableVertexAttribArray(program2DParamPosition);
    glVertexAttribPointer(program2DParamPosition, 3, GL_FLOAT, GL_FALSE, 0, pointerCoords.data());

    glDrawElements(GL_POINTS, 1, GL_UNSIGNED_BYTE, trivial2DData);
}

void Renderer::RenderCardboardAlignLine() {
    glEnableVertexAttribArray(program2DParamPosition);
    glVertexAttribPointer(program2DParamPosition, 3, GL_FLOAT, GL_FALSE, 0,
                          cardboardAlignLineCoords.data());

    glDrawElements(GL_LINES, 2, GL_UNSIGNED_BYTE, trivial2DData);
}

bool Renderer::UpdateDeviceParams() {
    // Checks if screen or device parameters changed
    if (!screenParamsChanged && !deviceParamsChanged) {
        return true;
    }

    // Get saved device parameters
    if (outputMode == OutputMode::CARDBOARD_STEREO) {
        uint8_t *cardboardQrCode;
        int size;
        CardboardQrCode_getSavedDeviceParams(&cardboardQrCode, &size);

        // If there are no parameters saved yet, returns false.
        if (size == 0) {
            LOG_ERROR("Cardboard params not available yet");
            return false;
        }

        cardboardLensDistortion = CardboardLensDistortionPointer(
                CardboardLensDistortion_create(cardboardQrCode, size, screenWidth, screenHeight)
        );

        CardboardQrCode_destroy(cardboardQrCode);
    }

    GlSetup();

    if (outputMode == OutputMode::CARDBOARD_STEREO) {
        const CardboardOpenGlEsDistortionRendererConfig config{kGlTexture2D};
        cardboardDistortionRenderer = CardboardDistortionRendererPointer(
                CardboardOpenGlEs2DistortionRenderer_create(&config)
        );

        CardboardMesh leftMesh;
        CardboardMesh rightMesh;
        CardboardLensDistortion_getDistortionMesh(cardboardLensDistortion.get(), kLeft, &leftMesh);
        CardboardLensDistortion_getDistortionMesh(cardboardLensDistortion.get(), kRight,
                                                  &rightMesh);

        CardboardDistortionRenderer_setMesh(cardboardDistortionRenderer.get(), &leftMesh, kLeft);
        CardboardDistortionRenderer_setMesh(cardboardDistortionRenderer.get(), &rightMesh, kRight);

        // Get eye matrices
        CardboardLensDistortion_getEyeFromHeadMatrix(cardboardLensDistortion.get(), kLeft,
                                                     glm::value_ptr(cardboardEyeMatrices[0]));
        CardboardLensDistortion_getEyeFromHeadMatrix(cardboardLensDistortion.get(), kRight,
                                                     glm::value_ptr(cardboardEyeMatrices[1]));
        CardboardLensDistortion_getProjectionMatrix(cardboardLensDistortion.get(), kRight, kzNear,
                                                    kzFar,
                                                    glm::value_ptr(cardboardProjectionMatrices[0]));
        CardboardLensDistortion_getProjectionMatrix(cardboardLensDistortion.get(), kRight, kzNear,
                                                    kzFar,
                                                    glm::value_ptr(cardboardProjectionMatrices[1]));
    }

    screenParamsChanged = false;
    deviceParamsChanged = false;

    CHECK_GL_ERROR("UpdateDeviceParams");

    return true;
}

void Renderer::GlSetup() {
    LOG_DEBUG("GLSetup");

    if (glInitialized) {
        GlTeardown();
    }
    glInitialized = true;

    // Create render texture.
    glGenTextures(1, &renderTexture);
    glBindTexture(GL_TEXTURE_2D, renderTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, screenWidth, screenHeight, 0, GL_RGB, GL_UNSIGNED_BYTE,
                 nullptr);

    // Create render target.
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderTexture, 0);
    CHECK_GL_ERROR("Create render buffer");

    cardboardEyeTextureDescriptions[0] = {
            .texture = renderTexture,
            .left_u = 0.0f,
            .right_u = 0.5f,
            .top_v = 1.0f,
            .bottom_v = 0.0f
    };
    cardboardEyeTextureDescriptions[1] = {
            .texture = renderTexture,
            .left_u = 0.5f,
            .right_u = 1.0f,
            .top_v = 1.0f,
            .bottom_v = 0.0f
    };

    CHECK_GL_ERROR("GlSetup");
}

void Renderer::GlTeardown() {
    if (!glInitialized) {
        return;
    }
    glInitialized = false;

    glDeleteFramebuffers(1, &framebuffer);
    framebuffer = 0;
    glDeleteTextures(1, &renderTexture);
    renderTexture = 0;

    CHECK_GL_ERROR("GlTeardown");
}

glm::mat4 Renderer::BuildMVPMatrix(int eye) {
    if (inputVideoMode == InputVideoMode::PLAIN_FOV && isOutputModeMono(outputMode)) {
        const float xScale = screenAspect > 1.0f ? 1.0f : screenAspect;
        const float yScale = screenAspect > 1.0f ? screenAspect : 1.0f;

        return glm::diagonal4x4(glm::vec4(
                xScale,
                yScale,
                1.0f,
                1.0f
        ));
    }

    glm::mat4 projection;
    glm::mat4 view;

    switch (outputMode) {
        case OutputMode::MONO_LEFT:
        case OutputMode::MONO_RIGHT:
            projection = glm::perspective(glm::radians(90.0f) / screenAspect, screenAspect, kzNear,
                                          kzFar);
            view = viewMatrix;
            break;

        case OutputMode::CARDBOARD_STEREO:
            view = cardboardEyeMatrices[eye] * viewMatrix;
            projection = cardboardProjectionMatrices[eye];
            break;

        default:
            assert(false);
    }

    return projection * view;
}

glm::mat4 Renderer::BuildColorMapMatrix(int eye) {
    if (inputVideoLayout != InputVideoLayout::ANAGLYPH_RED_CYAN) {
        return glm::mat4(1.0f);
    }

    assert(eye >= 0 && eye <= 1);
    switch (eye) {
        case 0:
            // red
            return {
                    1.0f, 1.0f, 1.0f, 0.0f,
                    0.0f, 0.0f, 0.0f, 0.0f,
                    0.0f, 0.0f, 0.0f, 0.0f,
                    0.0f, 0.0f, 0.0f, 1.0f,
            };

        case 1:
            // cyan
            return {
                    0.0f, 0.0f, 0.0f, 0.0f,
                    0.0f, 1.0f, 0.0f, 0.0f,
                    0.0f, 0.0f, 1.0f, 0.0f,
                    0.0f, 0.0f, 0.0f, 1.0f,
            };

        default:
            std::abort();
    }
}

void Renderer::SetOptions(InputVideoLayout requestedInputLayout, InputVideoMode requestedInputMode,
                          OutputMode requestedOutputMode) {
    LOG_DEBUG("SetOptions(%d, %d, %d)", requestedInputLayout, requestedInputMode,
              requestedOutputMode);
    deviceParamsChanged |= requestedOutputMode != this->outputMode;

    this->inputVideoLayout = requestedInputLayout;
    this->inputVideoMode = requestedInputMode;
    this->outputMode = requestedOutputMode;
    ComputeMesh();
}

void Renderer::ScanCardboardQr() {
    LOG_DEBUG("ScanCardboardQr");
    CardboardQrCode_scanQrCodeAndSaveDeviceParams();
}

void Renderer::ShowProgressBar() {
    LOG_DEBUG("ShowProgressBar");
    vrProgressBarShown = true;
    vrGuiProgressBarHideAt = time(nullptr) + PROGRESS_BAR_SHOW_TIME;
}

static TexturedMesh
BuildUvSphereMesh(int n_slices, int n_stacks, float minTheta, float maxTheta, float uvLeft,
                  float uvTop, float uvRight, float uvBottom) {
    TexturedMesh::Builder meshBuilder;

    float uvWidth = uvRight - uvLeft;
    float uvHeight = uvBottom - uvTop;
    float thetaRange = maxTheta - minTheta;

    // generate vertices per stack / slice
    for (int i = 0; i <= n_stacks; i++) {
        auto vFrac = float(i) / float(n_stacks);
        auto phi = float(M_PI) * vFrac;
        auto v = vFrac * uvHeight + uvTop;

        for (int j = 0; j <= n_slices; j++) {
            auto uFrac = float(j) / float(n_slices);
            auto theta = -(minTheta + thetaRange * uFrac);
            auto u = uFrac * uvWidth + uvLeft;
            auto x = std::sinf(phi) * std::sinf(theta);
            auto y = std::cosf(phi);
            auto z = std::sinf(phi) * std::cosf(theta);

            // texture correction for top- and bottom-layer vertices (collapsed into a point)
            if ((i == 0) || (i == n_stacks)) {
                u += 1.0f / float(n_slices);
                if (u > 1.0f) u -= 1.0f;
            }

            meshBuilder.add_vertex(x, y, z, u, v);
        }
    }

    // add quads per stack / slice
    for (int j = 0; j < n_stacks; j++) {
        auto j0 = j * (n_slices + 1);
        auto j1 = (j + 1) * (n_slices + 1);
        for (int i = 0; i < n_slices; i++) {
            auto i0 = j0 + i;
            auto i1 = j0 + (i + 1);
            auto i2 = j1 + (i + 1);
            auto i3 = j1 + i;
            // top- or bottom-layer are just triangles
            if (j == 0) {
                meshBuilder.add_triangle(i0, i2, i3);
            } else if (j == (n_stacks - 1)) {
                meshBuilder.add_triangle(i0, i2, i1);
            } else {
                // otherwise, quads
                meshBuilder.add_quad(i0, i1, i2, i3);
            }
        }
    }

    return meshBuilder.build();
}

static TexturedMesh
BuildCylindricalMesh(int n_slices, float minTheta, float maxTheta, float uvLeft, float uvTop,
                     float uvRight, float uvBottom) {
    TexturedMesh::Builder meshBuilder;

    float uvWidth = uvRight - uvLeft;
    float thetaRange = maxTheta - minTheta;

    for (int i = 0; i <= n_slices; i++) {
        auto uFrac = float(i) / float(n_slices);
        auto theta = -(minTheta + thetaRange * uFrac);
        auto u = uFrac * uvWidth + uvLeft;
        auto x = std::sinf(theta);
        auto z = std::cosf(theta);

        auto i1 = meshBuilder.add_vertex(x, 1, z, u, uvTop);
        auto i2 = meshBuilder.add_vertex(x, -1, z, u, uvBottom);
        if (i > 0) {
            meshBuilder.add_quad(i1 - 2, i1, i2, i2 - 2);
        }
    }

    return meshBuilder.build();
}

void Renderer::ComputeMesh() {
    for (int eye = 0; eye < 2; ++eye) {
        float uvLeft, uvTop, uvRight, uvBottom;
        switch (inputVideoLayout) {
            case InputVideoLayout::MONO:
            case InputVideoLayout::ANAGLYPH_RED_CYAN:
                uvLeft = 0.0f;
                uvRight = 1.0f;
                uvTop = 0.0f;
                uvBottom = 1.0f;
                break;

            case InputVideoLayout::STEREO_HORIZ:
                uvLeft = 0.5f * static_cast<float>(eye);
                uvRight = 0.5f * static_cast<float>(eye + 1);
                uvTop = 0.0f;
                uvBottom = 1.0f;
                break;

            case InputVideoLayout::STEREO_VERT:
                uvLeft = 0.0f;
                uvRight = 1.0f;
                uvTop = 0.5f * static_cast<float>(eye);
                uvBottom = 0.5f * static_cast<float>(eye + 1);
                break;

            default:
                assert(false);
        }

        switch (inputVideoMode) {
            case InputVideoMode::PLAIN_FOV: {
                // plain rectangle
                const float xScale = videoAspect > 1.0f ? 1.0f : (1.0f / videoAspect);
                const float yScale = videoAspect > 1.0f ? (1.0f / videoAspect) : 1.0f;

                std::unique_ptr<GLfloat[]> pos{new GLfloat[12]{
                        -xScale, +yScale, PLAIN_FOV_Z,
                        +xScale, +yScale, PLAIN_FOV_Z,
                        +xScale, -yScale, PLAIN_FOV_Z,
                        -xScale, -yScale, PLAIN_FOV_Z
                }};
                std::unique_ptr<GLfloat[]> uv{new GLfloat[8]{
                        uvLeft, uvTop,
                        uvRight, uvTop,
                        uvRight, uvBottom,
                        uvLeft, uvBottom
                }};
                std::unique_ptr<GLushort[]> indices{new GLushort[6]{
                        0, 2, 1,
                        0, 3, 2
                }};

                eyeMeshes[eye] =
                        TexturedMesh(
                                GL_TRIANGLES,
                                6,
                                std::move(pos),
                                std::move(uv),
                                std::move(indices)
                        );

                break;
            }

            case InputVideoMode::EQUIRECT_180:
                eyeMeshes[eye] = BuildUvSphereMesh(20, 20, M_PI_2, M_PI * 1.5f, uvLeft, uvTop,
                                                   uvRight, uvBottom);
                break;

            case InputVideoMode::EQUIRECT_360:
                eyeMeshes[eye] = BuildUvSphereMesh(40, 20, 0, M_PI * 2.0f, uvLeft, uvTop,
                                                   uvRight, uvBottom);
                break;

            case InputVideoMode::PANORAMA_180:
                eyeMeshes[eye] = BuildCylindricalMesh(20, M_PI_2, M_PI * 1.5f, uvLeft, uvTop,
                                                      uvRight, uvBottom);
                break;

            case InputVideoMode::PANORAMA_360:
                eyeMeshes[eye] = BuildCylindricalMesh(40, 0, M_PI * 2.0f, uvLeft, uvTop, uvRight,
                                                      uvBottom);
                break;

            default:
                std::unique_ptr<GLfloat[]> pos{new GLfloat[]{
                        -1.0, -1.0, -1.0,
                        +1.0, -1.0, -1.0,
                        +1.0, +1.0, -1.0,
                        -1.0, +1.0, -1.0,
                        -1.0, -1.0, +1.0,
                        +1.0, -1.0, +1.0,
                        +1.0, +1.0, +1.0,
                        -1.0, +1.0, +1.0
                }};
                std::unique_ptr<GLfloat[]> uv{new GLfloat[]{
                        0.0f, 0.0f,
                        1.0f, 0.0f,
                        1.0f, 1.0f,
                        0.0f, 1.0f,
                        0.0f, 0.0f,
                        1.0f, 0.0f,
                        1.0f, 1.0f,
                        0.0f, 1.0f,
                }};
                std::unique_ptr<GLushort[]> indices{new GLushort[]{
                        0, 5, 4, 0, 1, 5,
                        1, 6, 5, 1, 2, 6,
                        2, 7, 6, 2, 3, 7,
                        3, 4, 7, 3, 0, 4,
                        4, 6, 7, 4, 5, 6,
                        3, 1, 0, 3, 2, 1,
                }};

                eyeMeshes[eye] =
                        TexturedMesh(
                                GL_TRIANGLES,
                                36,
                                std::move(pos),
                                std::move(uv),
                                std::move(indices)
                        );
                break;
        }
    }
}

void Renderer::UpdatePose(JNIEnv *env) {
    glm::quat headOrientationQuat;
    glm::vec3 headPosition;
    CardboardHeadTracker_getPose(
            cardboardHeadTracker.get(),
            static_cast<int64_t>(GetBootTimeNano() + kPredictionTimeWithoutVsyncNanos),
            kLandscapeLeft,
            glm::value_ptr(headPosition),
            glm::value_ptr(headOrientationQuat)
    );

    // viewMatrix = glm::translate(toMat4(headOrientationQuat), headPosition);
    viewMatrix = glm::toMat4(headOrientationQuat);

    const glm::vec4 pointVector = NEG_Z_AXIS * viewMatrix;
    pitch = asinf(pointVector.y);
    if (pitch > 1.55f) { // roughly 88.8°
        // too vertical: just keep the previous yaw
    } else {
        yaw = -atan2f(pointVector.x, pointVector.z);
    }

    if (isHeadGesturingUp) {
        if (pitch < HEAD_GESTURE_PITCH_LIMIT_RETURN) {
            // head moved from up to not-up
            isHeadGesturingUp = false;
            LOG_DEBUG("UpdatePose: Moved from up to not-up");
        }
    } else {
        if (pitch > HEAD_GESTURE_PITCH_LIMIT) {
            // head moved from not-up to up
            isHeadGesturingUp = true;
            vrGuiShown = !vrGuiShown;
            vrGuiCenterTheta = yaw;
            LOG_DEBUG("UpdatePose: Moved from not-up to up");
        }
    }

    if (vrGuiShown) {
        for (VRGuiButton &button: vrGuiButtons) {
            ButtonAction hitAction = button.evaluatePossibleHit(M_PI + yaw - vrGuiCenterTheta,
                                                                pitch);
            if (hitAction != ButtonAction::NONE) {
                this->ExecuteButtonAction(hitAction, env);
            }
        }
    }
}

void Renderer::OnVideoSizeChanged(int width, int height) {
    videoWidth = width;
    videoHeight = height;
    videoAspect = (float) videoWidth / (float) videoHeight;
    screenParamsChanged = true;
}

void Renderer::ExecuteButtonAction(const ButtonAction action, JNIEnv *env) {
    switch (action) {
        case ButtonAction::NONE:
            // wat
            break;

        case ButtonAction::RECENTER_YAW:
            // TODO: Delay recenter?
            CardboardHeadTracker_recenter(cardboardHeadTracker.get());
            break;

        case ButtonAction::RECENTER_2D:
            break;

        default:
            javaInterface.ExecuteButtonAction(env, action);
            break;
    }
}

