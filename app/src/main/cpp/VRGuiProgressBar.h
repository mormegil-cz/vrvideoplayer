#ifndef VR_VIDEO_PLAYER_VRGUIPROGRESSBAR_H
#define VR_VIDEO_PLAYER_VRGUIPROGRESSBAR_H

#include <array>

#include <GLES2/gl2.h>

class VRGuiProgressBar {
public:
    VRGuiProgressBar(float xCenter, float yCenter, float width, float height);

    void render(GLint programParamPosition) const;
    void setProgress(float progress);

private:
    float xCenter;
    float yCenter;
    float width;
    float height;
    float progress;

    std::array<GLfloat, 12> vertexPos;
};


#endif //VR_VIDEO_PLAYER_VRGUIPROGRESSBAR_H
