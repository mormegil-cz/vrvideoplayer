#include "VRGuiProgressBar.h"

#include <GLES2/gl2.h>

VRGuiProgressBar::VRGuiProgressBar(float xCenter, float yCenter, float width, float height)
        : xCenter(xCenter),
          yCenter(yCenter),
          width(width),
          height(height),
          progress(0) {
}

void VRGuiProgressBar::render(GLint programParamPosition) const {

}

void VRGuiProgressBar::setProgress(float progress) {
    this->progress = progress;
}
