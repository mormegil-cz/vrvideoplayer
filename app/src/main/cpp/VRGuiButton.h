#ifndef VR_VIDEO_PLAYER_VRGUIBUTTON_H
#define VR_VIDEO_PLAYER_VRGUIBUTTON_H

#include <array>

#include <GLES2/gl2.h>
#include "glm/vec4.hpp"

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

enum class ButtonBehavior {
    DELAYED_TRIGGER,
    AUTO_REPEAT,
};

class VRGuiButton {
public:
    VRGuiButton(float centerTheta, float centerPhi, float centerDistance, float sizeAlpha, int textureXPos,
                int textureYPos, ButtonAction action, ButtonBehavior behavior, bool visible);

    void render(GLint programParamPosition, GLint programParamUV) const;

    ButtonAction evaluatePossibleHit(float viewTheta, float viewPhi);

    void setVisible(bool newVisible);

private:
    float centerTheta;
    float centerPhi;
    float sizeAlpha;
    ButtonAction action;
    ButtonBehavior behavior;
    bool visible;

    bool waitingForActivation;
    time_t activationTime;

    std::array<GLfloat, 12> vertexPos;
    std::array<GLfloat, 8> vertexUV;

    ButtonAction evaluateHit();
    ButtonAction doEnterButton(time_t now);
    ButtonAction doTriggerButton(time_t now);
};

#endif //VR_VIDEO_PLAYER_VRGUIBUTTON_H
