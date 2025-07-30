#include "Light.h"

Light::Light(unsigned int id, LightType type, bool drawLight)
    : ID(id), type(type), drawLight(drawLight), lightMesh(Mesh(MeshType::CUBE))
{
    std::cout << "[Lights] Created a Light with ID: " << ID << " and type: " << (unsigned int)type << std::endl;
}

void Light::Draw(Shader &lightShader)
{
    if (drawLight)
    {
        glm::mat4 model = glm::mat4(1.f);
        model = glm::translate(model, this->position);

        lightShader.setUniforms("uLightColor", static_cast<unsigned int>(UniformType::Vec3f), (void *)(glm::value_ptr(this->color)));
        lightShader.setUniforms("model", static_cast<unsigned int>(UniformType::Mat4f), (void *)(glm::value_ptr(model)));
        lightMesh.Draw(lightShader);
    }
}