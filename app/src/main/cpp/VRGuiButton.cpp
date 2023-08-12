#include "VRGuiButton.h"

#include <array>

#include <GLES2/gl2.h>

constexpr float BUTTON_TEXTURE_SIZE = 1024.0f;

static std::array<GLfloat, 12>
computeVertexPos(float centerTheta, float centerPhi, float centerDistance, float sizeAlpha) {

}

static std::array<GLfloat, 8> computeTexturePos(int textureXPos, int textureYPos) {
    float u0 = (float) textureXPos / BUTTON_TEXTURE_SIZE;
    float v0 = (float) textureYPos / BUTTON_TEXTURE_SIZE;
    float u1 = ((float) textureXPos + 255.0f) / BUTTON_TEXTURE_SIZE;
    float v1 = ((float) textureYPos + 255.0f) / BUTTON_TEXTURE_SIZE;
    return std::array<GLfloat, 8>{
            u0, v0,
            u1, v0,
            u1, v1,
            u0, v1
    };
}

VRGuiButton::VRGuiButton(float centerTheta, float centerPhi, float centerDistance, float sizeAlpha,
                         int textureXPos, int textureYPos, ButtonAction action, bool visible)
        : centerTheta(centerTheta),
          centerPhi(centerPhi),
          centerDistance(centerDistance),
          sizeAlpha(sizeAlpha),
          vertexPos(computeVertexPos(centerTheta, centerPhi, centerDistance, sizeAlpha)),
          vertexUV(computeTexturePos(textureXPos, textureYPos)),
          action(action),
          visible(visible) {
}

void VRGuiButton::Render() {

}
