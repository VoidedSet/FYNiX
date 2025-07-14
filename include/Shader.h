#pragma once

#include <fstream>
#include <sstream>
#include <string>
#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

enum class UniformType : uint8_t
{
    Int = 0x01,
    Bool = 0x0B,

    Float = 0x1F,
    Vec2f = 0x2F,
    Vec3f = 0x3F,
    Vec4f = 0x4F,

    Mat2f = 0x2A,
    Mat3f = 0x3A,
    Mat4f = 0x4A,
};

class Shader
{
public:
    unsigned int ID, vertexShaderID, fragmentShaderID;

    Shader(const char *vertex_shader_src, const char *fragment_shader_src);
    void createProgram();
    void use();

    void setUniforms(const char *uName, unsigned int type, void *value);

private:
    void checkCompileErrors(unsigned int id, const char *type);
};