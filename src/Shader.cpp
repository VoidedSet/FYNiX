#include "Shader.h"

using namespace std;

Shader::Shader(const char *vertex_shader_src, const char *fragment_shader_src)
{
    ifstream vFile(vertex_shader_src);
    ifstream fFile(fragment_shader_src);
    if (!vFile.is_open() || !fFile.is_open())
    {
        cerr << "[Shader - ERROR] Failed to open shader file(s)." << endl;
        return;
    }

    stringstream vStream, fStream;

    vStream << vFile.rdbuf();
    fStream << fFile.rdbuf();

    std::string vCode = vStream.str();
    std::string fCode = fStream.str();
    const char *vShaderCode = vCode.c_str();
    const char *fShaderCode = fCode.c_str();

    vertexShaderID = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShaderID, 1, &vShaderCode, NULL);
    glCompileShader(vertexShaderID);

    Shader::checkCompileErrors(vertexShaderID, "Shader");

    fragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShaderID, 1, &fShaderCode, NULL);
    glCompileShader(fragmentShaderID);

    Shader::checkCompileErrors(fragmentShaderID, "Shader");
}

void Shader::createProgram()
{
    ID = glCreateProgram();
    glAttachShader(ID, vertexShaderID);
    glAttachShader(ID, fragmentShaderID);
    glLinkProgram(ID);

    Shader::checkCompileErrors(ID, "Program");

    // delete the shaders as they're linked to the program now
    glDeleteShader(vertexShaderID);
    glDeleteShader(fragmentShaderID);
}

void Shader::checkCompileErrors(unsigned int id, const char *type)
{
    int success;
    char infoLog[512];

    if (type == "Shader")
    {
        glGetShaderiv(id, GL_COMPILE_STATUS, &success);

        if (!success)
        {
            glGetShaderInfoLog(id, 512, NULL, infoLog);
            cerr << "[Shader - ERROR] " << type << " Compilation Failed.\n"
                 << infoLog << endl;
        }
    }
    else if (type == "Program")
    {
        glGetProgramiv(id, GL_LINK_STATUS, &success);

        if (!success)
        {
            glGetProgramInfoLog(id, 512, NULL, infoLog);
            cerr << "[Shader - ERROR] " << type << " Linking Failed.\n"
                 << infoLog << endl;
        }
    }
}

void Shader::use()
{
    glUseProgram(ID);
}