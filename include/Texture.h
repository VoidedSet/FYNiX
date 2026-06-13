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
    int width;
    int height;
    int nrChannels;

    Texture();
    Texture(const char *filePath, GLenum textureType, unsigned int textureUnit, const std::string &typeName, bool uploadToGPU = true);
    
    void LoadCPU();
    void UploadToGPU();

    void Bind(unsigned int texSlot);
    void SetUniform(Shader &shader, const std::string &uniformName, unsigned int texSlot);
    void UnBind();

private:
    GLenum textureType;
    unsigned int textureUnit;
};