#pragma once

#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

class VertexBuffer
{
public:
    unsigned int ID;

    VertexBuffer(const void *data, unsigned int size);
    void Bind();
    void UnBind();
};