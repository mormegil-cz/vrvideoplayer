#include "VRGuiButton.h"

#include <cmath>
#include <array>

#include <GLES2/gl2.h>

constexpr float BUTTON_TEXTURE_SIZE = 1024.0f;

constexpr std::array<GLubyte, 4> quadFanIndices = {0, 1, 2, 3};

static std::array<GLfloat, 3> sphericalToCartesian(float theta, float phi, float r) {
    return {
            r * cosf(phi) * cosf(-theta),
            r * sinf(phi),
            r * cosf(phi) * sinf(-theta)
    };
}

static std::array<GLfloat, 12>
computeVertexPos(float centerTheta, float centerPhi, float centerDistance, float sizeAlpha) {
    float alpha2 = sizeAlpha * 0.5f;
    float vertexDistance = centerDistance / cosf(alpha2);
    std::array<GLfloat, 3> v0 = sphericalToCartesian(centerTheta - alpha2, centerPhi - alpha2,
                                                     vertexDistance);
    std::array<GLfloat, 3> v1 = sphericalToCartesian(centerTheta - alpha2, centerPhi + alpha2,
                                                     vertexDistance);
    std::array<GLfloat, 3> v2 = sphericalToCartesian(centerTheta + alpha2, centerPhi + alpha2,
                                                     vertexDistance);
    std::array<GLfloat, 3> v3 = sphericalToCartesian(centerTheta + alpha2, centerPhi - alpha2,
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
                         int textureXPos, int textureYPos, ButtonAction action, bool visible)
        : centerTheta(centerTheta),
          centerPhi(centerPhi),
          sizeAlpha(sizeAlpha),
          vertexPos(computeVertexPos(centerTheta, centerPhi, centerDistance, sizeAlpha)),
          vertexUV(computeTexturePos(textureXPos, textureYPos)),
          action(action),
          visible(visible) {
}

void VRGuiButton::Render(GLint programParamPosition, GLint programParamUV) const {
    if (!visible) return;

    glEnableVertexAttribArray(programParamPosition);
    glVertexAttribPointer(programParamPosition, 3, GL_FLOAT, GL_FALSE, 0, vertexPos.data());
    glEnableVertexAttribArray(programParamUV);
    glVertexAttribPointer(programParamUV, 2, GL_FLOAT, GL_FALSE, 0, vertexUV.data());

    glDrawElements(GL_TRIANGLE_FAN, 4, GL_UNSIGNED_BYTE, quadFanIndices.data());
    //CHECK_GL_ERROR("Render button");
}
