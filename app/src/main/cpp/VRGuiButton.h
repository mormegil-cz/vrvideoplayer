#ifndef VR_VIDEO_PLAYER_VRGUIBUTTON_H
#define VR_VIDEO_PLAYER_VRGUIBUTTON_H

#include <GLES2/gl2.h>

enum class ButtonAction {
    NONE,
    RECENTER_2D,
    RECENTER_YAW,
    VOLUME_DOWN,
    VOLUME_UP,
    OPEN_FILE,
    PLAY,
    BACK,
    FORWARD,
    REWIND,
    PAUSE
};

class VRGuiButton {
public:
    VRGuiButton(float centerTheta, float centerPhi, float centerDistance, float sizeAlpha, int textureXPos,
                int textureYPos, ButtonAction action, bool visible);

    void Render();

private:
    float centerTheta;
    float centerPhi;
    float centerDistance;
    float sizeAlpha;
    ButtonAction action;
    bool visible;

    std::array<GLfloat, 12> vertexPos;
    std::array<GLfloat, 8> vertexUV;
};

#endif //VR_VIDEO_PLAYER_VRGUIBUTTON_H
