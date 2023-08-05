#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/log.h>

#include <array>
#include <cmath>
#include <fstream>

#include <GLES2/gl2.h>

#include "Renderer.h"
#include "GLUtils.h"
#include "logger.h"

#define LOG_TAG "VrVideoPlayerR"

static GLint vertices[][3] = {
        {-0x10000, -0x10000, -0x10000},
        {0x10000,  -0x10000, -0x10000},
        {0x10000,  0x10000,  -0x10000},
        {-0x10000, 0x10000,  -0x10000},
        {-0x10000, -0x10000, 0x10000},
        {0x10000,  -0x10000, 0x10000},
        {0x10000,  0x10000,  0x10000},
        {-0x10000, 0x10000,  0x10000}
};

static GLint colors[][4] = {
        {0x00000, 0x00000, 0x00000, 0x10000},
        {0x10000, 0x00000, 0x00000, 0x10000},
        {0x10000, 0x10000, 0x00000, 0x10000},
        {0x00000, 0x10000, 0x00000, 0x10000},
        {0x00000, 0x00000, 0x10000, 0x10000},
        {0x10000, 0x00000, 0x10000, 0x10000},
        {0x10000, 0x10000, 0x10000, 0x10000},
        {0x00000, 0x10000, 0x10000, 0x10000}
};

static GLubyte indices[] = {
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
    in vec4 a_Position, a_Color;
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

void Renderer::drawFrame() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0, 0, -3.0f);
    glRotatef(angle, 0, 1, 0);
    glRotatef(angle * 0.25f, 1, 0, 0);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);

    glFrontFace(GL_CW);
    glVertexPointer(3, GL_FIXED, 0, vertices);
    glColorPointer(4, GL_FIXED, 0, colors);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_BYTE, indices);

    angle += 1.2f;
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

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    glUseProgram(obj_program);
    glVertexAttribPointer(obj_position_param, 2, GL_FLOAT, GL_FALSE, 0, triangleVertices);
    glEnableVertexAttribArray(vPosition);
    glDrawArrays(GL_TRIANGLES, 0, 3);

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

    /*
    if (framebuffer != 0) {
        GlTeardown();
    }
    */

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

    /*
    // Generate depth buffer to perform depth test.
    glGenRenderbuffers(1, &depthRenderBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depthRenderBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, screen_width, screen_height);
    CHECK_GL_ERROR("Create Render buffer");
    */

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
    /*
    if (framebuffer == 0) {
        return;
    }
    glDeleteRenderbuffers(1, &depthRenderBuffer);
    depthRenderBuffer = 0;
    glDeleteFramebuffers(1, &framebuffer);
    framebuffer = 0;
    glDeleteTextures(1, &texture);
    texture = 0;
    */

    CHECK_GL_ERROR("GlTeardown");
}
