#ifndef VR_VIDEO_PLAYER_TEXTUREDMESH_H
#define VR_VIDEO_PLAYER_TEXTUREDMESH_H

#include <memory>
#include <vector>

#include <GLES2/gl2.h>

class TexturedMesh {
public:
    TexturedMesh();

    TexturedMesh(GLsizei vertexCount,
                 std::unique_ptr<GLfloat[]> vertexPos,
                 std::unique_ptr<GLfloat[]> vertexUV,
                 std::unique_ptr<GLushort[]> vertexIndex);

    void Render(GLint programParamPosition, GLint programParamUV) const;

    class Builder {
    public:
        GLushort add_vertex(float x, float y, float z, float u, float v);
        void add_triangle(GLushort a, GLushort b, GLushort c);
        void add_quad(GLushort a, GLushort b, GLushort c, GLushort d);

        TexturedMesh build();

    private:
        std::vector<GLfloat> vertexPos;
        std::vector<GLfloat> vertexUV;
        std::vector<GLushort> vertexIndex;
    };

private:
    GLsizei vertexCount;
    std::unique_ptr<GLfloat[]> vertexPos;
    std::unique_ptr<GLfloat[]> vertexUV;
    std::unique_ptr<GLushort[]> vertexIndex;
};

#endif //VR_VIDEO_PLAYER_TEXTUREDMESH_H
