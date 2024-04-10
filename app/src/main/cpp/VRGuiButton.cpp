#include "VRGuiButton.h"

#include <cmath>
#include <array>

#include <GLES2/gl2.h>

#include "logger.h"

#define LOG_TAG "VRVideoPlayerB"

constexpr float BUTTON_TEXTURE_SIZE = 1024.0f;
constexpr int ACTIVATION_DELAY = 3;

constexpr std::array<GLubyte, 4> quadFanIndices = {0, 1, 2, 3};

static std::array<GLfloat, 3> sphericalToCartesian(float theta, float phi, float r) {
    return {
            r * cosf(phi) * sinf(-theta),
            r * sinf(phi),
            r * cosf(phi) * cosf(-theta)
    };
}

static std::array<GLfloat, 12>
computeVertexPos(float centerTheta, float centerPhi, float centerDistance, float sizeAlpha) {
    float alpha2 = sizeAlpha * 0.5f;
    float vertexDistance = centerDistance / cosf(alpha2);
    std::array<GLfloat, 3> v0 = sphericalToCartesian(centerTheta - alpha2, centerPhi + alpha2,
                                                     vertexDistance);
    std::array<GLfloat, 3> v1 = sphericalToCartesian(centerTheta - alpha2, centerPhi - alpha2,
                                                     vertexDistance);
    std::array<GLfloat, 3> v2 = sphericalToCartesian(centerTheta + alpha2, centerPhi - alpha2,
                                                     vertexDistance);
    std::array<GLfloat, 3> v3 = sphericalToCartesian(centerTheta + alpha2, centerPhi + alpha2,
                                                     vertexDistance);
    return {
            v0[0], v0[1], v0[2],
            v1[0], v1[1], v1[2],
            v2[0], v2[1], v2[2],
            v3[0], v3[1], v3[2]
    };
}

static std::array<GLfloat, 8> computeTexturePos(int textureXPos, int textureYPos) {
    float u0 = (float) textureXPos / BUTTON_TEXTURE_SIZE;
    float v0 = (float) textureYPos / BUTTON_TEXTURE_SIZE;
    float u1 = ((float) textureXPos + 255.0f) / BUTTON_TEXTURE_SIZE;
    float v1 = ((float) textureYPos + 255.0f) / BUTTON_TEXTURE_SIZE;
    return {
            u0, v0,
            u0, v1,
            u1, v1,
            u1, v0
    };
}

VRGuiButton::VRGuiButton(float centerTheta, float centerPhi, float centerDistance, float sizeAlpha,
                         int textureXPos, int textureYPos, ButtonAction action,
                         ButtonBehavior behavior, bool visible)
        : centerTheta(centerTheta),
          centerPhi(centerPhi),
          sizeAlpha(sizeAlpha),
          vertexPos(computeVertexPos(centerTheta, centerPhi, centerDistance, sizeAlpha)),
          vertexUV(computeTexturePos(textureXPos, textureYPos)),
          action(action),
          behavior(behavior),
          visible(visible) {
}

void VRGuiButton::render(GLint programParamPosition, GLint programParamUV) const {
    if (!visible) return;

    glEnableVertexAttribArray(programParamPosition);
    glVertexAttribPointer(programParamPosition, 3, GL_FLOAT, GL_FALSE, 0, vertexPos.data());
    glEnableVertexAttribArray(programParamUV);
    glVertexAttribPointer(programParamUV, 2, GL_FLOAT, GL_FALSE, 0, vertexUV.data());

    glDrawElements(GL_TRIANGLE_FAN, 4, GL_UNSIGNED_BYTE, quadFanIndices.data());
    //CHECK_GL_ERROR("Render button");
}

ButtonAction VRGuiButton::evaluatePossibleHit(float viewTheta, float viewPhi) {
    if (!visible) return ButtonAction::NONE;

    if ((fabsf(viewTheta - centerTheta) * 2.0f < sizeAlpha) &&
        (fabsf((viewPhi - centerPhi) * 2.0f) < sizeAlpha)) {
        return evaluateHit();
    }

    if (waitingForActivation) {
        LOG_DEBUG("Left button %d", action);
        waitingForActivation = false;
    }
    return ButtonAction::NONE;
}

ButtonAction VRGuiButton::evaluateHit() {
    time_t now = time(nullptr);
    if (waitingForActivation) {
        if (now >= activationTime) {
            LOG_DEBUG("Button %d triggered", action);
            return doTriggerButton(now);
        }
        // still waiting
        return ButtonAction::NONE;
    } else {
        LOG_DEBUG("Entered button %d", action);
        return doEnterButton(now);
    }
}

void VRGuiButton::setVisible(bool newVisible) {
    this->visible = newVisible;
}

ButtonAction VRGuiButton::doEnterButton(time_t now) {
    switch (behavior) {
        case ButtonBehavior::AUTO_REPEAT:
            return doTriggerButton(now);

        case ButtonBehavior::DELAYED_TRIGGER:
            waitingForActivation = true;
            activationTime = now + ACTIVATION_DELAY;
            return ButtonAction::NONE;
    }
}

ButtonAction VRGuiButton::doTriggerButton(time_t now) {
    switch (behavior) {
        case ButtonBehavior::AUTO_REPEAT:
            waitingForActivation = true;
            activationTime = now + ACTIVATION_DELAY;
            return action;

        case ButtonBehavior::DELAYED_TRIGGER:
            waitingForActivation = false;
            return action;
    }
}
