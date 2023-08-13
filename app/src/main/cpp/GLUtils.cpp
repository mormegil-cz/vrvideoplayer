#include "GLUtils.h"

#include <cmath>
#include <ctime>

#include <array>
#include <random>
#include <sstream>
#include <string>
#include <utility>

#include <android/log.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include "logger.h"

#define LOG_TAG "VRVideoPlayerU"

namespace {
    class RunAtEndOfScope {
    public:
        explicit RunAtEndOfScope(std::function<void()> function) : function(std::move(function)) {}

        ~RunAtEndOfScope() { function(); }
    private:
        std::function<void()> function;
    };
}

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
        glGetShaderInfoLog(shader, info_len, nullptr, info_string.data());
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

bool LoadPngFromAssetManager(JNIEnv *env, jobject java_asset_mgr, int target,
                             const std::string &path) {
    jclass bitmap_factory_class =
            env->FindClass("android/graphics/BitmapFactory");
    jclass asset_manager_class =
            env->FindClass("android/content/res/AssetManager");
    jclass gl_utils_class = env->FindClass("android/opengl/GLUtils");
    jmethodID decode_stream_method = env->GetStaticMethodID(
            bitmap_factory_class, "decodeStream",
            "(Ljava/io/InputStream;)Landroid/graphics/Bitmap;");
    jmethodID open_method = env->GetMethodID(
            asset_manager_class, "open", "(Ljava/lang/String;)Ljava/io/InputStream;");
    jmethodID tex_image_2d_method = env->GetStaticMethodID(
            gl_utils_class, "texImage2D", "(IILandroid/graphics/Bitmap;I)V");

    jstring j_path = env->NewStringUTF(path.c_str());
    RunAtEndOfScope cleanup_j_path([&] {
        if (j_path) {
            env->DeleteLocalRef(j_path);
        }
    });

    jobject image_stream =
            env->CallObjectMethod(java_asset_mgr, open_method, j_path);
    jobject image_obj = env->CallStaticObjectMethod(
            bitmap_factory_class, decode_stream_method, image_stream);
    if (env->ExceptionOccurred() != nullptr) {
        LOG_ERROR("Java exception while loading image");
        env->ExceptionClear();
        return false;
    }

    env->CallStaticVoidMethod(gl_utils_class, tex_image_2d_method, target, 0,
                              image_obj, 0);
    return true;
}

bool InitVideoTexturePlayback(JNIEnv *env, jobject javaVideoTexturePlayer, GLuint textureName) {
    jclass videoTexturePlayerClazz = env->FindClass("cz/mormegil/vrvideoplayer/VideoTexturePlayer");
    jmethodID initializeVideoTexturePlayerMethod = env->GetMethodID(videoTexturePlayerClazz,
                                                                    "initializePlayback",
                                                                    "(I)V");

    if (textureName > INT32_MAX) {
        // ??!?
        LOG_ERROR("Invalid texture name");
        return false;
    }
    jint textureNameJava = static_cast<jint>(textureName);

    env->CallVoidMethod(javaVideoTexturePlayer, initializeVideoTexturePlayerMethod,
                        textureNameJava);

    if (env->ExceptionOccurred() != nullptr) {
        LOG_ERROR("Java exception while preparing video texture");
        env->ExceptionClear();
        return false;
    }

    return true;
}

static constexpr uint64_t kNanosInSeconds = 1000000000;

uint64_t GetBootTimeNano() {
    struct timespec res{};
    clock_gettime(CLOCK_BOOTTIME, &res);
    return (res.tv_sec * kNanosInSeconds) + res.tv_nsec;
}
