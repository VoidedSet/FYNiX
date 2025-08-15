#include "ShaderManager.h"

ShaderManager::ShaderManager()
{
    std::cout << "[ShaderManager] Initializing Shaders." << std::endl;
}

void ShaderManager::addShader(const std::string &shaderName, const char *vertex_path, const char *fragment_path)
{
    // find is much faster than a loop
    if (shaderMap.find(shaderName) != shaderMap.end())
    {
        std::cerr << "[ShaderManager] Shader program with name: " << shaderName << " already exists!" << std::endl;
        return;
    }

    std::cout << "[ShaderManager] Creating shader program with name: " << shaderName << std::endl;

    // Create the shader directly inside the map to avoid copies
    shaderMap.emplace(shaderName, Shader(shaderName.c_str(), vertex_path, fragment_path));
    shaderMap.at(shaderName).createProgram();
}

Shader &ShaderManager::findShader(const std::string &shaderName)
{
    try
    {
        // .at() is a fast lookup and throws an exception if not found
        return shaderMap.at(shaderName);
    }
    catch (const std::out_of_range &e)
    {
        std::cerr << "[ShaderManager] Shader program with name: " << shaderName << " not found!" << std::endl;
        throw; // Re-throw the exception
    }
}