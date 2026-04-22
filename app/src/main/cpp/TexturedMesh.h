#ifndef VR_VIDEO_PLAYER_TEXTUREDMESH_H
#define VR_VIDEO_PLAYER_TEXTUREDMESH_H

#include <memory>
#include <vector>

#include <GLES2/gl2.h>

class TexturedMesh {
public:
    TexturedMesh();

    TexturedMesh(GLenum mode,
                 size_t vertexDataCount,
                 GLsizei indexCount,
                 std::unique_ptr<GLfloat[]> vertexData,
                 std::unique_ptr<GLushort[]> vertexIndex);

    void UploadBufferObjects();
    void DestroyBufferObjects();
    void Render(GLint programParamPosition, GLint programParamUV) const;

    class Builder {
    public:
        GLushort add_vertex(float x, float y, float z, float u, float v);
        void add_triangle(GLushort a, GLushort b, GLushort c);
        void add_quad(GLushort a, GLushort b, GLushort c, GLushort d);

        TexturedMesh build();

    private:
        std::vector<GLfloat> vertexData;
        std::vector<GLushort> indices;
    };

private:
    GLenum mode;
    size_t vertexDataCount;
    GLsizei indexCount;
    GLuint vbo;
    GLuint ebo;
    GLuint vao;
    std::unique_ptr<GLfloat[]> vertexData;
    std::unique_ptr<GLushort[]> indices;
};

#endif //VR_VIDEO_PLAYER_TEXTUREDMESH_H
