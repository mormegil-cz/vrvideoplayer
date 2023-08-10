#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/log.h>

#include <array>
#include <cmath>
#include <fstream>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include "glm/mat4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/scalar_constants.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "Renderer.h"
#include "GLUtils.h"
#include "logger.h"

#define LOG_TAG "VRVideoPlayerR"


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
in vec2 v_UV;
out vec4 fragColor;

void main() {
  fragColor = texture(u_Texture, v_UV);
})glsl";

Renderer::Renderer(JavaVM *vm, jobject javaContextObj, jobject javaAssetMgrObj,
                   jobject javaVideoTexturePlayerObj)
        : glInitialized(false),
          frameCount(0),
          inputVideoMode(InputVideoMode::EQUIRECT_180),
          inputVideoLayout(InputVideoLayout::STEREO_HORIZ),
          outputMode(OutputMode::MONO_LEFT),
          eyeMeshes{} {
    LOG_DEBUG("Renderer instance created");

    JNIEnv *env;
    vm->GetEnv((void **) &env, JNI_VERSION_1_6);
    javaContext = env->NewGlobalRef(javaContextObj);
    javaAssetMgr = env->NewGlobalRef(javaAssetMgrObj);
    javaVideoTexturePlayer = env->NewGlobalRef(javaVideoTexturePlayerObj);

    SetOptions(InputVideoLayout::STEREO_HORIZ, InputVideoMode::EQUIRECT_180, OutputMode::MONO_LEFT);
}

Renderer::~Renderer() {
    LOG_DEBUG("Renderer instance destroyed");

    // TODO: deallocate javaContext, javaAssetMgr, javaVideoTexturePlayer?
}

void Renderer::OnPause() {
    LOG_DEBUG("OnPause after %lu frames", frameCount);
}

void Renderer::OnResume() {
    LOG_DEBUG("OnResume");

    frameCount = 0;

    // Parameters may have changed.
    deviceParamsChanged = true;
}

void Renderer::SetScreenParams(int width, int height) {
    LOG_DEBUG("SetScreenParams(%d, %d)", width, height);

    screenWidth = width;
    screenHeight = height;
    screenParamsChanged = true;
}

static void
initTexture(JNIEnv *env, jobject java_asset_mgr, GLuint &textureId, const std::string &path) {
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

static void bindTexture(GLuint textureId) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId);
}

static void initVideoTexture(JNIEnv *env, jobject javaVideoTexturePlayer, GLuint &textureId) {
    glGenTextures(1, &textureId);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, textureId);

    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    if (!InitVideoTexturePlayback(env, javaVideoTexturePlayer, textureId)) {
        LOG_ERROR("Couldn't initialize video texture");
        return;
    }
    CHECK_GL_ERROR("Video texture init");
}

static void bindVideoTexture(GLuint textureId) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, textureId);
}

void Renderer::OnSurfaceCreated(JNIEnv *env) {
    LOG_DEBUG("OnSurfaceCreated");

    const GLuint obj_vertex_shader = LoadGLShader(GL_VERTEX_SHADER, kVertexShader);
    const GLuint obj_fragment_shader = LoadGLShader(GL_FRAGMENT_SHADER, kFragmentShader);

    program = glCreateProgram();
    glAttachShader(program, obj_vertex_shader);
    glAttachShader(program, obj_fragment_shader);
    glLinkProgram(program);
    glUseProgram(program);
    CHECK_GL_ERROR("Obj program");

    programParamPosition = glGetAttribLocation(program, "a_Position");
    programParamUV = glGetAttribLocation(program, "a_UV");
    programParamMVPMatrix = glGetUniformLocation(program, "u_MVP");
    CHECK_GL_ERROR("Obj program params");

    // initTexture(env, javaAssetMgr, videoTexture, "test-image-square.png");
    initVideoTexture(env, javaVideoTexturePlayer, videoTexture);
}

void Renderer::DrawFrame() {
    if (!UpdateDeviceParams()) {
        return;
    }

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glDisable(GL_SCISSOR_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClear(GL_COLOR_BUFFER_BIT);
    CHECK_GL_ERROR("Params");

    // bindTexture(videoTexture);
    bindVideoTexture(videoTexture);
    CHECK_GL_ERROR("Bind texture");

    glUseProgram(program);

    int minEye, maxEye;
    switch (outputMode) {
        case OutputMode::MONO_LEFT:
            minEye = 0;
            maxEye = 0;
            break;
        case OutputMode::MONO_RIGHT:
            minEye = 1;
            maxEye = 1;
            break;
        case OutputMode::CARDBOARD_STEREO:
            minEye = 0;
            maxEye = 1;
            break;
        default:
            assert(false);
    }

    bool isMono = minEye == maxEye;
    for (int eye = minEye; eye <= maxEye; ++eye) {
        glViewport(0, 0, screenWidth, screenHeight);

        auto mvpMatrix = BuildMVPMatrix();
        glUniformMatrix4fv(programParamMVPMatrix, 1, GL_FALSE, glm::value_ptr(mvpMatrix));

        eyeMeshes[eye].Render(programParamPosition, programParamUV);
    }

    ++frameCount;
}

bool Renderer::UpdateDeviceParams() {
    // Checks if screen or device parameters changed
    if (!screenParamsChanged && !deviceParamsChanged) {
        return true;
    }

    GlSetup();

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

    /*
    // Generate depth buffer to perform depth test.
    glGenRenderbuffers(1, &depthRenderBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depthRenderBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, screenWidth, screenHeight);
    CHECK_GL_ERROR("Create depth buffer");
    */


    /*
    // Create render texture.
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, screenWidth, screenHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

    // Create render target.
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,
                              depthRenderBuffer);
    CHECK_GL_ERROR("Create render buffer");
    */

    CHECK_GL_ERROR("GlSetup");
}

void Renderer::GlTeardown() {
    if (!glInitialized) {
        return;
    }
    glInitialized = false;

    /*
    glDeleteRenderbuffers(1, &depthRenderBuffer);
    depthRenderBuffer = 0;
    */
    /*
    glDeleteFramebuffers(1, &framebuffer);
    framebuffer = 0;
    glDeleteTextures(1, &texture);
    texture = 0;
    */

    CHECK_GL_ERROR("GlTeardown");
}

glm::mat4 Renderer::BuildMVPMatrix() {
    auto aspect = (float) screenWidth / (float) screenHeight;
    glm::mat4 projection = glm::perspective(glm::radians(90.0f) / aspect, aspect, 0.1f, 10.0f);

    glm::mat4 view = glm::lookAt(
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 0.0f, -1.0f),
            glm::vec3(0.0f, 1.0f, 0.0f)
    );

    /*
    glm::mat4 model = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, angle * 0.25f, glm::vec3(1.0f, 0.0f, 0.0f));
    */

    return projection * view;
}

void Renderer::SetOptions(InputVideoLayout layout, InputVideoMode inputMode,
                          OutputMode outputMode) {
    this->inputVideoLayout = layout;
    this->inputVideoMode = inputMode;
    this->outputMode = outputMode;
    ComputeMesh();
}

static TexturedMesh
BuildUvSphereMesh(int n_slices, int n_stacks, float minTheta, float maxTheta, float uvLeft,
                  float uvTop, float uvRight, float uvBottom) {
    TexturedMesh::Builder meshBuilder;

    float uvWidth = uvRight - uvLeft;
    float uvHeight = uvBottom - uvTop;

    // generate vertices per stack / slice
    for (int i = 0; i <= n_stacks; i++) {
        auto vFrac = float(i) / float(n_stacks);
        auto phi = float(M_PI) * vFrac;
        auto v = vFrac * uvHeight + uvTop;

        for (int j = 0; j <= n_slices; j++) {
            auto uFrac = float(j) / float(n_slices);
            auto theta = -(minTheta + (maxTheta - minTheta) * uFrac);
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

void Renderer::ComputeMesh() {
    for (int eye = 0; eye < 2; ++eye) {
        float uvLeft, uvTop, uvRight, uvBottom;
        switch (inputVideoLayout) {
            case InputVideoLayout::MONO:
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
                std::unique_ptr<GLfloat[]> pos{new GLfloat[12]{
                        -1.0f, +1.0f, -1.0f,
                        +1.0f, +1.0f, -1.0f,
                        +1.0f, -1.0f, -1.0f,
                        -1.0f, -1.0f, -1.0f
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
                eyeMeshes[eye] = BuildUvSphereMesh(20, 20, 0, M_2_PI, uvLeft, uvTop,
                                                   uvRight, uvBottom);
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
                                36,
                                std::move(pos),
                                std::move(uv),
                                std::move(indices)
                        );
                break;
        }
    }
}
