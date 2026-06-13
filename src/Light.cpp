#include "Light.h"
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

Light::Light(unsigned int id, LightType type)
    : ID(id), type(type), lightMesh(type == LightType::SPOT ? Mesh(MeshType::CONE) : Mesh(MeshType::SPHERE))
{
    std::cout << "[Lights] Created a Light with ID: " << ID << " and type: " << (unsigned int)type << std::endl;
}

void Light::Draw(Shader &lightShader)
{
    glm::mat4 model = glm::mat4(1.f);
    model = glm::translate(model, this->position);
    model = glm::scale(model, glm::vec3(.3f));

    lightShader.setUniforms("uLightColor", static_cast<unsigned int>(UniformType::Vec3f), (void *)(glm::value_ptr(this->color)));
    lightShader.setUniforms("model", static_cast<unsigned int>(UniformType::Mat4f), (void *)(glm::value_ptr(model)));
    lightMesh.Draw(lightShader);
}