#include "VertexArray.h"

VertexArray::VertexArray()
{
    glGenVertexArrays(1, &ID);
    glBindVertexArray(ID);
}

// Index = attribute location (e.g., 0)
// Count = number of components (e.g., 3 for vec3)
void VertexArray::AddAttribLayout(unsigned int index, int count, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer)
{
    glVertexAttribPointer(index, count, type, normalized, stride, pointer);
    glEnableVertexAttribArray(index);
}

void VertexArray::Bind()
{
    glBindBuffer(GL_ARRAY_BUFFER, ID);
}

void VertexArray::UnBind()
{
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}