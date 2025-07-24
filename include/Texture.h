#pragma once

#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "Shader.h"

class Texture
{
public:
    unsigned int ID;
    unsigned char *data;

    std::string path;
    std::string type;

    Texture(const char *filePath, GLenum textureType, unsigned int textureUnit, const std::string &typeName);
    void Bind(unsigned int texSlot);
    void SetUniform(Shader &shader, const std::string &uniformName);
    void UnBind();

private:
    GLenum textureType;
    unsigned int textureUnit;
};