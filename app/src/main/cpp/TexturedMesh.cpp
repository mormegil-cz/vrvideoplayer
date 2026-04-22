#define GL_GLEXT_PROTOTYPES

#include "TexturedMesh.h"
#include "logger.h"

#define LOG_TAG "VRVideoPlayerM"

#include <cassert>

#include <utility>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

TexturedMesh::TexturedMesh() :
        vertexDataCount(0),
        indexCount(0),
        mode{},
        vertexData{},
        indices{},
        vbo(0),
        ebo(0),
        vao(0) {
}

TexturedMesh::TexturedMesh(GLenum mode,
                           size_t vertexDataCount,
                           GLsizei indexCount,
                           std::unique_ptr<GLfloat[]> vertexData,
                           std::unique_ptr<GLushort[]> indices) :
        mode(mode),
        vertexDataCount(vertexDataCount),
        indexCount(indexCount),
        vertexData(std::move(vertexData)),
        indices(std::move(indices)),
        vbo(0),
        ebo(0),
        vao(0) {
    assert(vertexDataCount * sizeof(vertexData[0]) <= std::numeric_limits<GLsizei>::max());
    assert(indexCount * sizeof(indices[0]) <= std::numeric_limits<GLsizei>::max());
}

void TexturedMesh::UploadBufferObjects() {
    if (vertexDataCount == 0) {
        // uninitialized/empty mesh
        LOG_DEBUG("Empty mesh");
        return;
    }
    if (vbo || ebo || vao) {
        LOG_ERROR("Buffer objects already uploaded!");
    }

    GLuint buffers[2];
    glGenBuffers(2, buffers);
    vbo = buffers[0];
    ebo = buffers[1];
    glGenVertexArraysOES(1, &vao);

    glBindVertexArrayOES(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(vertexData[0]) * vertexDataCount),
                 vertexData.get(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                          (const void *) (0 * sizeof(float)));
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                          (const void *) (3 * sizeof(float)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(indices[0]) * indexCount),
                 indices.get(), GL_STATIC_DRAW);

    glBindVertexArrayOES(GL_NONE);
}

void TexturedMesh::DestroyBufferObjects() {
    if (ebo) {
        glDeleteBuffers(1, &ebo);
        ebo = 0;
    }
    if (vbo) {
        glDeleteBuffers(1, &vbo);
        vbo = 0;
    }
    if (vao) {
        glDeleteVertexArraysOES(1, &vao);
        vao = 0;
    }
}

void TexturedMesh::Render(GLint programParamPosition, GLint programParamUV) const {
    if (vertexDataCount == 0) {
        // uninitialized/empty mesh
        return;
    }
    if (!vao) {
        LOG_ERROR("VAO is not ready!");
        return;
    }

    glBindVertexArrayOES(vao);
    glDrawElements(mode, indexCount, GL_UNSIGNED_INT, nullptr);

    glBindVertexArrayOES(GL_NONE);
    //CHECK_GL_ERROR("Render");
}

GLushort TexturedMesh::Builder::add_vertex(float x, float y, float z, float u, float v) {
    std::size_t size = vertexData.size();
    assert((size % 5) == 0);
    assert((size / 5) <= std::numeric_limits<GLushort>::max());
    auto index = static_cast<GLushort>(size / 5);

    vertexData.push_back(x);
    vertexData.push_back(y);
    vertexData.push_back(z);
    vertexData.push_back(u);
    vertexData.push_back(v);

    return index;
}

void TexturedMesh::Builder::add_triangle(GLushort a, GLushort b, GLushort c) {
    indices.push_back(a);
    indices.push_back(b);
    indices.push_back(c);
}

void TexturedMesh::Builder::add_quad(GLushort a, GLushort b, GLushort c, GLushort d) {
    indices.push_back(a);
    indices.push_back(c);
    indices.push_back(b);

    indices.push_back(a);
    indices.push_back(d);
    indices.push_back(c);
}

TexturedMesh TexturedMesh::Builder::build() {
    std::size_t indexCountSizeT = indices.size();
    assert(indexCountSizeT <= std::numeric_limits<GLsizei>::max());

    std::unique_ptr<GLfloat[]> dataPtr = std::make_unique<GLfloat[]>(vertexData.size());
    memcpy(dataPtr.get(), vertexData.data(), sizeof(vertexData[0]) * vertexData.size());
    std::unique_ptr<GLushort[]> indPtr = std::make_unique<GLushort[]>(indices.size());
    memcpy(indPtr.get(), indices.data(), sizeof(indices[0]) * indices.size());

    return {
            GL_TRIANGLES,
            vertexData.size(),
            static_cast<GLsizei>(indexCountSizeT),
            std::move(dataPtr),
            std::move(indPtr)
    };
}
