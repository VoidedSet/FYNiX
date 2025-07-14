#include "Texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace std;

Texture::Texture(const char *filePath, GLenum textureType, unsigned int textureUnit)
{
    Texture::textureType = textureType;
    Texture::textureUnit = textureUnit;

    glGenTextures(1, &ID);
    glBindTexture(textureType, ID);

    glTexParameteri(textureType, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(textureType, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(textureType, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(textureType, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_set_flip_vertically_on_load(true);
    int width, height, nrChannels;
    unsigned char *data = stbi_load(filePath, &width, &height, &nrChannels, 0);
    GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
    if (!data)
    {
        cerr << "[Texture - ERROR] Failed to load the texture at " << filePath << endl;
        return;
    }
    else
    {
        glTexImage2D(textureType, 0, format, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(textureType);
    }

    stbi_image_free(data);
}

void Texture::Bind()
{
    glActiveTexture(GL_TEXTURE0 + textureUnit);
    glBindTexture(textureType, ID);
}

void Texture::UnBind()
{
    glBindTexture(textureType, 0);
}