#ifndef VR_VIDEO_PLAYER_TEXTUREDMESH_H
#define VR_VIDEO_PLAYER_TEXTUREDMESH_H

#include <memory>

#include <GLES2/gl2.h>

class TexturedMesh {
public:
    TexturedMesh();

    TexturedMesh(GLsizei vertexCount,
                 std::unique_ptr<GLfloat[]> vertexPos,
                 std::unique_ptr<GLfloat[]> vertexUV,
                 std::unique_ptr<GLushort[]> vertexIndex);

    void Render(GLint programParamPosition, GLint programParamUV);

private:
    GLsizei vertexCount;
    std::unique_ptr<GLfloat[]> vertexPos;
    std::unique_ptr<GLfloat[]> vertexUV;
    std::unique_ptr<GLushort[]> vertexIndex;
};


#endif //VR_VIDEO_PLAYER_TEXTUREDMESH_H
