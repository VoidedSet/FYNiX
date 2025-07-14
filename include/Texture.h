#pragma once

#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

class Texture
{
public:
    unsigned int ID;
    unsigned char *data;

    Texture(const char *filePath, GLenum textureType, unsigned int textureUnit);
    void Bind();
    void UnBind();

private:
    GLenum textureType;
    unsigned int textureUnit;
};