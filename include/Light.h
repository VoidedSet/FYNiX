#pragma once

#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Mesh.h"

enum LightType
{
    DIRECTIONAL,
    POINTLIGHT,
    SPOT
};

class Light
{
public:
    unsigned int ID;
    glm::vec3 position;
    glm::vec3 color = glm::vec3(1.f);
    LightType type;

    bool drawLight;
    Mesh lightMesh;

    Light(unsigned int id, LightType type, bool drawLight = true);
    // Light(unsigned int id, LightType type, bool drawModel = true, std::string modelPath);

    void Draw(Shader &lightShader);
};