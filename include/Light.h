#pragma once

#include <glm/glm.hpp>

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

    Mesh lightMesh;

    Light(unsigned int id, LightType type);

    void Draw(Shader &lightShader);
};