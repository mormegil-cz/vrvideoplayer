#include "GLUtils.h"
#include "logger.h"

#include <android/log.h>

#include <array>
#include <cmath>
#include <random>
#include <sstream>
#include <string>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#define LOG_TAG "VrVideoPlayerU"

GLuint LoadGLShader(GLenum type, const char *shader_source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &shader_source, nullptr);
    glCompileShader(shader);

    // Get the compilation status.
    GLint compile_status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compile_status);

    // If the compilation failed, delete the shader and show an error.
    if (compile_status == 0) {
        GLint info_len = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_len);
        if (info_len == 0) {
            return 0;
        }

        std::vector<char> info_string(info_len);
        glGetShaderInfoLog(shader, info_string.size(), nullptr, info_string.data());
        LOG_ERROR("Could not compile shader of type %d: %s", type, info_string.data());
        glDeleteShader(shader);
        return 0;
    } else {
        return shader;
    }
}

void CheckGlError(const char *file, int line, const char *label) {
    bool hasError = false;
    for (GLenum gl_error = glGetError(); gl_error != GL_NO_ERROR; gl_error = glGetError()) {
        hasError = true;
        LOG_ERROR("%s : %d > GL error @ %s: %d", file, line, label, gl_error);
    }
    if (hasError) {
        // Crash immediately to make OpenGL errors obvious.
        abort();
    }
}
