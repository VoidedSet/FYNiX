#pragma once

// #include <vector>
#include <map>

#include "Shader.h"

class ShaderManager
{
public:
    // std::vector<Shader> shaders;
    // std::vector<std::string> shaderNames;

    ShaderManager();

    void addShader(const std::string &shaderName, const char *vertex_shader_src, const char *fragment_shader_src);

    bool deleteShader(const char *shaderName);

    Shader &findShader(const std::string &shaderName);

    // void Update();

    void listShaders()
    {
        unsigned int i = 0;

        std::cout << "[ShaderManager] Listing all the available shaders." << std::endl;
        for (const auto &pair : shaderMap)
            std::cout << ++i << ") " << pair.first << std::endl;
    }

private:
    std::map<std::string, Shader> shaderMap;
    unsigned int shaderCount = 0;
};