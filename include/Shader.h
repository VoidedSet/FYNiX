#pragma once

#include <fstream>
#include <sstream>
#include <string>
#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

class Shader
{
public:
    unsigned int ID, vertexShaderID, fragmentShaderID;

    Shader(const char *vertex_shader_src, const char *fragment_shader_src);
    void createProgram();
    void use();

private:
    void checkCompileErrors(unsigned int id, const char *type);
};