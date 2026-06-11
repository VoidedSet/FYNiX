#pragma once

#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

class VertexBuffer
{
public:
    unsigned int ID;

    VertexBuffer() : ID(0) {}
    VertexBuffer(const void *data, unsigned int size);

    void Init(const void *data, unsigned int size);

    void Bind();
    void UnBind();
};