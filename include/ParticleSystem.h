#pragma once

#include <vector>
#include <glm/glm.hpp>

#include "BufferObjects/VertexBuffer.h"
#include "BufferObjects/VertexArray.h"
#include "shader.h"

#include "Light.h"

// Represents a single particle
struct Particle
{
    glm::vec3 Position, Velocity;
    glm::vec4 Color;
    float Life;
    float Size;
    Particle()
        : Position(0.0f), Velocity(0.0f), Color(1.0f), Life(0.0f) {}
};

class ParticleEmitter
{
public:
    unsigned int ID, maxParticles;
    glm::vec3 Position = glm::vec3(0.f);
    glm::vec4 Color = glm::vec4(1.0f, 0.5f, 0.2f, 1.0f);
    Shader shader;

    ParticleEmitter(Shader shader, unsigned int maxParticles);
    ParticleEmitter(Shader shader, unsigned int maxParticles, unsigned int ID);

    void Update(float dt);
    void Draw();

    void SpawnParticle(Particle particle);

private:
    std::vector<Particle> particles;
    unsigned int lastUsedParticle;

    std::vector<float> particleData;

    unsigned int VAO;
    unsigned int instanceVBO; // VBO for instance data

    void init();

    unsigned int firstUnusedParticle();
};
