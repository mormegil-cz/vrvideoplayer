#ifndef VR_VIDEO_PLAYER_GLUTILS_H
#define VR_VIDEO_PLAYER_GLUTILS_H

#include <android/asset_manager.h>
#include <jni.h>

#include <array>
#include <vector>

#include <GLES2/gl2.h>

GLuint LoadGLShader(GLenum type, const char *shader_source);

void CheckGlError(const char *file, int line, const char *label);

#define CHECK_GL_ERROR(label) CheckGlError(__FILE__, __LINE__, label)

bool LoadPngFromAssetManager(JNIEnv *env, jobject java_asset_mgr, int target, const std::string &path);

#endif //VR_VIDEO_PLAYER_GLUTILS_H
