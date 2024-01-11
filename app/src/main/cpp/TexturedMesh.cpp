#include "TexturedMesh.h"

#include <cassert>

#include <utility>

#include <GLES2/gl2.h>

TexturedMesh::TexturedMesh() :
        vertexCount(0),
        mode{},
        vertexPos{},
        vertexUV{},
        vertexIndex{} {
}

TexturedMesh::TexturedMesh(GLenum mode,
                           GLsizei vertexCount,
                           std::unique_ptr<GLfloat[]> vertexPos,
                           std::unique_ptr<GLfloat[]> vertexUV,
                           std::unique_ptr<GLushort[]> vertexIndex) :
        mode(mode),
        vertexCount(vertexCount),
        vertexPos(std::move(vertexPos)),
        vertexUV(std::move(vertexUV)),
        vertexIndex(std::move(vertexIndex)) {
}

void TexturedMesh::Render(GLint programParamPosition, GLint programParamUV) const {
    if (vertexCount == 0) {
        // uninitialized/empty mesh
        return;
    }

    glEnableVertexAttribArray(programParamPosition);
    glVertexAttribPointer(programParamPosition, 3, GL_FLOAT, GL_FALSE, 0, vertexPos.get());
    glEnableVertexAttribArray(programParamUV);
    glVertexAttribPointer(programParamUV, 2, GL_FLOAT, GL_FALSE, 0, vertexUV.get());

    glDrawElements(mode, vertexCount, GL_UNSIGNED_SHORT, vertexIndex.get());
    //CHECK_GL_ERROR("render");
}

GLushort TexturedMesh::Builder::add_vertex(float x, float y, float z, float u, float v) {
    std::size_t size = vertexPos.size();
    assert((size % 3) == 0);
    assert((size / 3) <= std::numeric_limits<GLushort>::max());
    auto index = static_cast<GLushort>(size / 3);

    vertexPos.push_back(x);
    vertexPos.push_back(y);
    vertexPos.push_back(z);

    vertexUV.push_back(u);
    vertexUV.push_back(v);

    return index;
}

void TexturedMesh::Builder::add_triangle(GLushort a, GLushort b, GLushort c) {
    vertexIndex.push_back(a);
    vertexIndex.push_back(b);
    vertexIndex.push_back(c);
}

void TexturedMesh::Builder::add_quad(GLushort a, GLushort b, GLushort c, GLushort d) {
    vertexIndex.push_back(a);
    vertexIndex.push_back(c);
    vertexIndex.push_back(b);

    vertexIndex.push_back(a);
    vertexIndex.push_back(d);
    vertexIndex.push_back(c);
}

TexturedMesh TexturedMesh::Builder::build() {
    std::size_t size = vertexIndex.size();
    assert(size <= std::numeric_limits<GLsizei>::max());

    std::unique_ptr<GLfloat[]> posPtr = std::make_unique<GLfloat[]>(vertexPos.size());
    memcpy(posPtr.get(), vertexPos.data(), sizeof(vertexPos[0]) * vertexPos.size());
    std::unique_ptr<GLfloat[]> uvPtr = std::make_unique<GLfloat[]>(vertexUV.size());
    memcpy(uvPtr.get(), vertexUV.data(), sizeof(vertexUV[0]) * vertexUV.size());
    std::unique_ptr<GLushort[]> indPtr = std::make_unique<GLushort[]>(vertexIndex.size());
    memcpy(indPtr.get(), vertexIndex.data(), sizeof(vertexIndex[0]) * vertexIndex.size());

    return {
            GL_TRIANGLES,
            static_cast<GLsizei>(size),
            std::move(posPtr),
            std::move(uvPtr),
            std::move(indPtr)
    };
}
