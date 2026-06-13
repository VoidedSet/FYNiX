#include "Texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <regex>

using namespace std;

// utility function needed for this file only

std::string decodeURIComponent(const std::string &str)
{
    std::string ret = str;
    ret = std::regex_replace(ret, std::regex("%20"), " ");

    // cout << "[Texture] Decoded file path: " << ret << endl;
    return ret;
}

Texture::Texture()
    : ID(0), data(nullptr), path(""), type(""), width(0), height(0), nrChannels(0), textureType(GL_TEXTURE_2D), textureUnit(0)
{
}

Texture::Texture(const char *filePath, GLenum textureType, unsigned int textureUnit, const std::string &typeName, bool uploadToGPU)
    : ID(0), data(nullptr), path(decodeURIComponent(filePath)), type(typeName), width(0), height(0), nrChannels(0), textureType(textureType), textureUnit(textureUnit)
{
    LoadCPU();
    if (uploadToGPU)
    {
        UploadToGPU();
    }
}

void Texture::LoadCPU()
{
    if (data)
        return;

    stbi_set_flip_vertically_on_load(false);
    data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);
    if (!data)
    {
        std::cerr << "[Texture - ERROR] Failed to load: " << path << std::endl;
    }
}

void Texture::UploadToGPU()
{
    if (!data)
        return;

    glGenTextures(1, &ID);
    glBindTexture(textureType, ID);

    glTexParameteri(textureType, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(textureType, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(textureType, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(textureType, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;

    glTexImage2D(textureType, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(textureType);

    stbi_image_free(data);
    data = nullptr;
}

void Texture::Bind(unsigned int texSlot)
{
    glActiveTexture(GL_TEXTURE0 + texSlot);
    glBindTexture(textureType, ID);
}

void Texture::SetUniform(Shader &shader, const std::string &uniformName, unsigned int texSlot)
{
    shader.use(); // Optional, only if not already active
    int loc = shader.getUniformLocation(uniformName);
    if (loc == -1)
    {
        std::cerr << "[Texture - ERROR] Uniform '" << uniformName << "' not found in shader." << std::endl;
        return;
    }
    glUniform1i(loc, texSlot);
}

void Texture::UnBind()
{
    glBindTexture(textureType, 0);
}