#include "Shader.h"

using namespace std;

Shader::Shader(const char *Name, const char *vertex_shader_src, const char *fragment_shader_src)
{
    this->Name = Name;
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

    // cout << "[Shader] Shader program with vert shader at " << vertex_shader_src << " and fragment shader at " << fragment_shader_src << " was loaded! Waiting to create program." << endl;
}

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

    // cout << "[Shader] Shader program with vert shader at " << vertex_shader_src << " and fragment shader at " << fragment_shader_src << " was loaded! Waiting to create program." << endl;
}

void Shader::createProgram()
{
    ID = glCreateProgram();
    glAttachShader(ID, vertexShaderID);
    glAttachShader(ID, fragmentShaderID);
    glLinkProgram(ID);

    Shader::checkCompileErrors(ID, "Program");

    // cout << "Shader program: " << Name << " created.";

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

void Shader::setUniforms(const char *uName, unsigned int type, void *value)
{
    int location = glGetUniformLocation(ID, uName);
    if (location == -1)
    {
        cerr << "[Shader - ERROR] Uniform '" << uName << "' not found." << endl;
        return;
    }

    switch (static_cast<UniformType>(type))
    {
    case UniformType::Float:
        glUniform1f(location, *static_cast<float *>(value));
        break;

    case UniformType::Int:
        glUniform1i(location, *static_cast<int *>(value));
        break;

    case UniformType::Bool:
        glUniform1i(location, *static_cast<bool *>(value));
        break;

    case UniformType::Vec2f:
        glUniform2fv(location, 1, static_cast<float *>(value));
        break;

    case UniformType::Vec3f:
        glUniform3fv(location, 1, static_cast<float *>(value));
        break;

    case UniformType::Vec4f:
        glUniform4fv(location, 1, static_cast<float *>(value));
        break;

    case UniformType::Mat2f:
        glUniformMatrix2fv(location, 1, GL_FALSE, static_cast<float *>(value));
        break;

    case UniformType::Mat3f:
        glUniformMatrix3fv(location, 1, GL_FALSE, static_cast<float *>(value));
        break;

    case UniformType::Mat4f:
        glUniformMatrix4fv(location, 1, GL_FALSE, static_cast<float *>(value));
        break;

    default:
        cerr << "[Shader - ERROR]: Unknown uniform type.\n";
        break;
    }
}
