#pragma once

#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

class ElementBuffer
{
public:
    unsigned int ID;

    ElementBuffer(const void *data, unsigned int size);
    void AddAttribLayout(unsigned int index, int count, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer);
    void Bind();
    void UnBind();
};