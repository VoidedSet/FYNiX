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

Texture::Texture(const char *filePath, GLenum textureType, unsigned int textureUnit, const std::string &typeName)
{
    std::string decodedPath = decodeURIComponent(filePath);

    this->textureType = textureType;
    this->textureUnit = textureUnit;
    this->path = decodedPath;
    this->type = typeName;

    glGenTextures(1, &ID);
    glBindTexture(textureType, ID);

    glTexParameteri(textureType, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(textureType, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(textureType, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(textureType, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width, height, nrChannels;
    unsigned char *data = stbi_load(decodedPath.c_str(), &width, &height, &nrChannels, 0);

    GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;

    if (!data)
    {
        std::cerr << "[Texture - ERROR] Failed to load: " << decodedPath << std::endl;
        return;
    }

    stbi_set_flip_vertically_on_load(false);

    glTexImage2D(textureType, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(textureType);

    stbi_image_free(data);
}

void Texture::Bind(unsigned int texSlot)
{
    glActiveTexture(GL_TEXTURE0 + texSlot);
    glBindTexture(textureType, ID);
}

void Texture::SetUniform(Shader &shader, const std::string &uniformName)
{
    shader.use(); // Optional, only if not already active
    int loc = glGetUniformLocation(shader.ID, uniformName.c_str());
    if (loc == -1)
    {
        std::cerr << "[Texture - ERROR] Uniform '" << uniformName << "' not found in shader." << std::endl;
        return;
    }
    glUniform1i(loc, textureUnit);
}

void Texture::UnBind()
{
    glBindTexture(textureType, 0);
}