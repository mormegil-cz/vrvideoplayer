#include "TexturedMesh.h"

#include <utility>

TexturedMesh::TexturedMesh() :
        vertexCount(0),
        vertexPos{},
        vertexUV{},
        vertexIndex{} {
}

TexturedMesh::TexturedMesh(GLsizei vertexCount,
                           std::unique_ptr<GLfloat[]> vertexPos,
                           std::unique_ptr<GLfloat[]> vertexUV,
                           std::unique_ptr<GLushort[]> vertexIndex) :
        vertexCount(vertexCount),
        vertexPos(std::move(vertexPos)),
        vertexUV(std::move(vertexUV)),
        vertexIndex(std::move(vertexIndex)) {
}

void TexturedMesh::Render(GLint programParamPosition, GLint programParamUV) {
    if (vertexCount == 0) {
        // uninitialized/empty mesh
        return;
    }

    glEnableVertexAttribArray(programParamPosition);
    glVertexAttribPointer(programParamPosition, 3, GL_FLOAT, GL_FALSE, 0, vertexPos.get());
    glEnableVertexAttribArray(programParamUV);
    glVertexAttribPointer(programParamUV, 2, GL_FLOAT, GL_FALSE, 0, vertexUV.get());
//    glEnableVertexAttribArray(programParamColor);
//    glVertexAttribPointer(programParamColor, 4, GL_FLOAT, GL_FALSE, 0, vertexColor.get());

    glDrawElements(GL_TRIANGLES, vertexCount, GL_UNSIGNED_SHORT, vertexIndex.get());
    //CHECK_GL_ERROR("Render");
}
