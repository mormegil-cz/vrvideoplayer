#include "VRGuiProgressBar.h"

#include <GLES2/gl2.h>

static constexpr GLubyte line2DData[] = {0, 1};

VRGuiProgressBar::VRGuiProgressBar(float xCenter, float yCenter, float width, float height)
        : xCenter(xCenter),
          yCenter(yCenter),
          width(width),
          height(height),
          progress(0) {
}

void VRGuiProgressBar::render(GLint program2DParamPosition) const {
    std::array<float, 6> progressLineCoords = {xCenter - 0.5f * width, yCenter, 0.5f,
                                               xCenter + width * (progress - 0.5f), yCenter, 0.5f};

    glEnableVertexAttribArray(program2DParamPosition);
    glVertexAttribPointer(program2DParamPosition, 3, GL_FLOAT, GL_FALSE, 0,
                          progressLineCoords.data());

    glDrawElements(GL_LINES, 2, GL_UNSIGNED_BYTE, line2DData);
}

void VRGuiProgressBar::setProgress(float progress) {
    this->progress = progress;
}
