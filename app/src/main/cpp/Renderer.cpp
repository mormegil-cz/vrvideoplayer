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

static GLfloat objVertices[] = {
        -1.0, -1.0, -1.0,
        +1.0, -1.0, -1.0,
        +1.0, +1.0, -1.0,
        -1.0, +1.0, -1.0,
        -1.0, -1.0, +1.0,
        +1.0, -1.0, +1.0,
        +1.0, +1.0, +1.0,
        -1.0, +1.0, +1.0
};

static GLfloat objUV[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f,
};

static GLfloat objColors[] = {
        0.00, 0.00, 0.00, 1.00,
        1.00, 0.00, 0.00, 1.00,
        1.00, 1.00, 0.00, 1.00,
        0.00, 1.00, 0.00, 1.00,
        0.00, 0.00, 1.00, 1.00,
        1.00, 0.00, 1.00, 1.00,
        1.00, 1.00, 1.00, 1.00,
        0.00, 1.00, 1.00, 1.00,
};

static GLubyte objIndices[] = {
        0, 4, 5, 0, 5, 1,
        1, 5, 6, 1, 6, 2,
        2, 6, 7, 2, 7, 3,
        3, 7, 4, 3, 4, 0,
        4, 7, 6, 4, 6, 5,
        3, 0, 1, 3, 1, 2,
};


constexpr const char *kVertexShader = R"glsl(#version 300 es
uniform mat4 u_MVP;
in vec4 a_Position, a_Color;
in vec2 a_UV;
out vec2 v_UV;
out vec4 v_Color;

void main() {
  v_UV = a_UV;
  v_Color = a_Color;
  gl_Position = u_MVP * a_Position;
})glsl";

constexpr const char *kFragmentShader = R"glsl(#version 300 es
#extension GL_OES_EGL_image_external : enable
#extension GL_OES_EGL_image_external_essl3 : enable
precision mediump float;

uniform samplerExternalOES u_Texture;
in vec2 v_UV;
in vec4 v_Color;
out vec4 fragColor;

void main() {
  fragColor = texture(u_Texture, v_UV) * v_Color;
})glsl";

Renderer::Renderer(JavaVM *vm, jobject obj, jobject javaAssetMgrObj, jobject javaVideoTexturePlayerObj)
        : glInitialized(false),
          angle(0),
          frameCount(0) {
    LOG_DEBUG("Renderer instance created");

    JNIEnv *env;
    vm->GetEnv((void **) &env, JNI_VERSION_1_6);
    javaAssetMgr = env->NewGlobalRef(javaAssetMgrObj);
    javaVideoTexturePlayer = env->NewGlobalRef(javaVideoTexturePlayerObj);
}

Renderer::~Renderer() {
    LOG_DEBUG("Renderer instance destroyed");

    // TODO: deallocate javaAssetMgr, javaVideoTexturePlayer?
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

static void initTexture(JNIEnv *env, jobject java_asset_mgr, GLuint &textureId, const std::string &path) {
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

static void initVideoTexture(JNIEnv *env, jobject javaVideoTexturePlayer, GLuint &textureId, const std::string &path) {
    glGenTextures(1, &textureId);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, textureId);

    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    if (!InitVideoTexturePlayback(env, javaVideoTexturePlayer, textureId, path)) {
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

    obj_program = glCreateProgram();
    glAttachShader(obj_program, obj_vertex_shader);
    glAttachShader(obj_program, obj_fragment_shader);
    glLinkProgram(obj_program);
    glUseProgram(obj_program);
    CHECK_GL_ERROR("Obj program");

    obj_position_param = glGetAttribLocation(obj_program, "a_Position");
    obj_uv_param = glGetAttribLocation(obj_program, "a_UV");
    obj_color_param = glGetAttribLocation(obj_program, "a_Color");
    obj_modelview_projection_param = glGetUniformLocation(obj_program, "u_MVP");
    CHECK_GL_ERROR("Obj program params");

    // initTexture(env, javaAssetMgr, cubeTexture, "test-image-square.png");
    initVideoTexture(env, javaVideoTexturePlayer, cubeTexture, "video-texture.webm");
}

void Renderer::DrawFrame() {
    if (!UpdateDeviceParams()) {
        return;
    }

    glViewport(0, 0, screenWidth, screenHeight);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CW); // TODO: Swap orientation and remove this
    glDisable(GL_SCISSOR_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    CHECK_GL_ERROR("Params");

    // bindTexture(cubeTexture);
    bindVideoTexture(cubeTexture);
    CHECK_GL_ERROR("Bind texture");

    glUseProgram(obj_program);

    auto mvpMatrix = BuildMVPMatrix();
    glUniformMatrix4fv(obj_modelview_projection_param, 1, GL_FALSE, glm::value_ptr(mvpMatrix));

    glEnableVertexAttribArray(obj_position_param);
    glVertexAttribPointer(obj_position_param, 3, GL_FLOAT, GL_FALSE, 0, objVertices);
    glEnableVertexAttribArray(obj_uv_param);
    glVertexAttribPointer(obj_uv_param, 2, GL_FLOAT, GL_FALSE, 0, objUV);
    glEnableVertexAttribArray(obj_color_param);
    glVertexAttribPointer(obj_color_param, 4, GL_FLOAT, GL_FALSE, 0, objColors);

    glDrawElements(GL_TRIANGLES, (sizeof objIndices) / (sizeof objIndices[0]), GL_UNSIGNED_BYTE,
                   objIndices);
    CHECK_GL_ERROR("Render");

    angle += glm::radians(1.2f);
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
    // Create render texture.
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    */

    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, screenWidth, screenHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

    // Generate depth buffer to perform depth test.
    glGenRenderbuffers(1, &depthRenderBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depthRenderBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, screenWidth, screenHeight);
    CHECK_GL_ERROR("Create Render buffer");

    /*
    // Create render target.
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,
                              depthRenderBuffer);
    */

    CHECK_GL_ERROR("GlSetup");
}

void Renderer::GlTeardown() {
    if (!glInitialized) {
        return;
    }
    glInitialized = false;

    glDeleteRenderbuffers(1, &depthRenderBuffer);
    depthRenderBuffer = 0;
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
    glm::mat4 projection = glm::perspective(glm::radians(90.0f), aspect, 1.0f, 10.0f);

    glm::mat4 view = glm::lookAt(
            glm::vec3(0.0f, 0.0f, -3.0f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f)
    );

    glm::mat4 model = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, angle * 0.25f, glm::vec3(1.0f, 0.0f, 0.0f));

    return projection * view * model;
}
