#pragma once

#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

class VertexArray
{
public:
    unsigned int ID;

    VertexArray();
    void AddAttribLayout(unsigned int index, int count, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer);
    void Bind();
    void UnBind();
};