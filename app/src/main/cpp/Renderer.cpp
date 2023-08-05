#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/log.h>

#include <array>
#include <cmath>
#include <fstream>

#include <GLES2/gl2.h>

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

#define LOG_TAG "VrVideoPlayerR"

static GLfloat objVertices[][3] = {
        {-0x10000, -0x10000, -0x10000},
        {0x10000,  -0x10000, -0x10000},
        {0x10000,  0x10000,  -0x10000},
        {-0x10000, 0x10000,  -0x10000},
        {-0x10000, -0x10000, 0x10000},
        {0x10000,  -0x10000, 0x10000},
        {0x10000,  0x10000,  0x10000},
        {-0x10000, 0x10000,  0x10000}
};

static GLfloat objUV[][2] = {
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
        {0.0f, 1.0f},
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
        {0.0f, 1.0f}
};

static GLint objColors[][4] = {
        {0x00000, 0x00000, 0x00000, 0x10000},
        {0x10000, 0x00000, 0x00000, 0x10000},
        {0x10000, 0x10000, 0x00000, 0x10000},
        {0x00000, 0x10000, 0x00000, 0x10000},
        {0x00000, 0x00000, 0x10000, 0x10000},
        {0x10000, 0x00000, 0x10000, 0x10000},
        {0x10000, 0x10000, 0x10000, 0x10000},
        {0x00000, 0x10000, 0x10000, 0x10000}
};

static GLubyte objIndices[] = {
        0, 4, 5, 0, 5, 1,
        1, 5, 6, 1, 6, 2,
        2, 6, 7, 2, 7, 3,
        3, 7, 4, 3, 4, 0,
        4, 7, 6, 4, 6, 5,
        3, 0, 1, 3, 1, 2
};


constexpr const char *kVertexShader =
        R"glsl(
    #version 300 es

    uniform mat4 u_MVP;
    in vec3 a_Position;
    in vec4 a_Color;
    in vec2 a_UV;
    out vec2 v_UV;
    out vec4 v_Color;

    void main() {
      v_UV = a_UV;
      v_Color = a_Color;
      gl_Position = u_MVP * a_Position;
    })glsl";

constexpr const char *kFragmentShader =
        R"glsl(
    #version 300 es
    precision mediump float;

    in sampler2D u_Texture;
    in vec2 v_UV;
    in vec4 v_Color;

    void main() {
      gl_FragColor = texture2D(u_Texture, vec2(v_UV.x, v_UV.y)) * v_Color;
    })glsl";

Renderer::Renderer(JavaVM *vm, jobject obj, jobject asset_mgr_obj, jobject java_app_obj)
        : java_app_obj(java_app_obj),
          java_asset_mgr(asset_mgr_obj),
          angle(0) {
    LOG_DEBUG("Renderer instance created");

    JNIEnv *env;
    vm->GetEnv((void **) &env, JNI_VERSION_1_6);
    java_asset_mgr = env->NewGlobalRef(asset_mgr_obj);
    this->java_app_obj = env->NewGlobalRef(java_app_obj);
    this->asset_mgr = AAssetManager_fromJava(env, asset_mgr_obj);
}

Renderer::~Renderer() {
    LOG_DEBUG("Renderer instance destroyed");
}

void Renderer::OnPause() {
    LOG_DEBUG("OnPause");
}

void Renderer::OnResume() {
    LOG_DEBUG("OnResume");

    // Parameters may have changed.
    device_params_changed = true;
}

void Renderer::SetScreenParams(int width, int height) {
    LOG_DEBUG("SetScreenParams(%d, %d)", width, height);

    screen_width = width;
    screen_height = height;
    screen_params_changed = true;
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
}

void Renderer::DrawFrame() {
    if (!UpdateDeviceParams()) {
        return;
    }

    glViewport(0, 0, screen_width, screen_height);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glDisable(GL_SCISSOR_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(obj_program);

    auto mvpMatrix = BuildMVPMatrix();
    glUniformMatrix4fv(obj_modelview_projection_param, 1, GL_FALSE, glm::value_ptr(mvpMatrix));

    glEnableVertexAttribArray(obj_position_param);
    glVertexAttribPointer(obj_position_param, 3, GL_FLOAT, false, 0, objVertices);
    glEnableVertexAttribArray(obj_uv_param);
    glVertexAttribPointer(obj_uv_param, 2, GL_FLOAT, false, 0, objUV);
    glEnableVertexAttribArray(obj_color_param);
    glVertexAttribPointer(obj_color_param, 4, GL_FLOAT, false, 0, objColors);

    glDrawElements(GL_TRIANGLES, (sizeof objIndices) / (sizeof objIndices[0]), GL_UNSIGNED_SHORT,
                   objIndices);

    angle += 1.2f;
}

bool Renderer::UpdateDeviceParams() {
    // Checks if screen or device parameters changed
    if (!screen_params_changed && !device_params_changed) {
        return true;
    }

    GlSetup();

    screen_params_changed = false;
    device_params_changed = false;

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

    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, screen_width, screen_height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

    // Generate depth buffer to perform depth test.
    glGenRenderbuffers(1, &depthRenderBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depthRenderBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, screen_width, screen_height);
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
    auto aspect = (float) screen_width / (float) screen_height;
    glm::mat4 projection = glm::perspective(glm::pi<float>() * 0.25f, aspect, 1.f, 100.f);
    glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -3.0f));
    view = glm::rotate(view, angle, glm::vec3(0.0f, 1.0f, 0.0f));
    view = glm::rotate(view, angle * 0.25f, glm::vec3(1.0f, 0.0f, 0.0f));
    return projection * view;
}
