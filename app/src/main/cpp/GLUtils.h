#ifndef VR_VIDEO_PLAYER_GLUTILS_H
#define VR_VIDEO_PLAYER_GLUTILS_H

#include <array>
#include <vector>

#include <jni.h>
#include <android/asset_manager.h>
#include <GLES2/gl2.h>

#include <cardboard.h>

GLuint LoadGLShader(GLenum type, const char *shader_source);

void CheckGlError(const char *file, int line, const char *label);

#define CHECK_GL_ERROR(label) CheckGlError(__FILE__, __LINE__, label)

bool
LoadPngFromAssetManager(JNIEnv *env, jobject java_asset_mgr, int target, const std::string &path);

bool InitVideoTexturePlayback(JNIEnv *env, jobject javaVideoTexturePlayer, GLuint textureName);

uint64_t GetBootTimeNano();

struct CardboardHeadTrackerDeleter {
    void operator()(CardboardHeadTracker *p) const { CardboardHeadTracker_destroy(p); }
};

using CardboardHeadTrackerPointer = std::unique_ptr<CardboardHeadTracker, CardboardHeadTrackerDeleter>;

struct CardboardLensDistortionDeleter {
    void operator()(CardboardLensDistortion *p) const { CardboardLensDistortion_destroy(p); }
};

using CardboardLensDistortionPointer = std::unique_ptr<CardboardLensDistortion, CardboardLensDistortionDeleter>;

struct CardboardDistortionRendererDeleter {
    void operator()(CardboardDistortionRenderer *p) const {
        CardboardDistortionRenderer_destroy(p);
    }
};

using CardboardDistortionRendererPointer = std::unique_ptr<CardboardDistortionRenderer, CardboardDistortionRendererDeleter>;

#endif //VR_VIDEO_PLAYER_GLUTILS_H
