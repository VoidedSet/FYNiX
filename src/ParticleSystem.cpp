#include "ParticleSystem.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>

ParticleEmitter::ParticleEmitter(Shader shader, unsigned int maxParticles)
    : shader(shader), maxParticles(maxParticles), lastUsedParticle(0)
{
    this->particles.resize(maxParticles);

    this->particleData.resize(maxParticles * 8);

    init();
}

ParticleEmitter::ParticleEmitter(Shader shader, unsigned int maxParticles, unsigned int ID)
    : shader(shader), maxParticles(maxParticles), lastUsedParticle(0), ID(ID)
{
    this->particles.resize(maxParticles);

    this->particleData.resize(maxParticles * 8);

    init();

    std::cout << "[ParticleSystem] Created a new Particle emitter with shader: " << shader.Name << std::endl;
}

void ParticleEmitter::init()
{
    float particle_quad[] = {
        // positions  // texCoords
        -0.5f, 0.5f, 0.0f, 1.0f,  // top-left
        -0.5f, -0.5f, 0.0f, 0.0f, // bottom-left
        0.5f, 0.5f, 1.0f, 1.0f,   // top-right
        0.5f, -0.5f, 1.0f, 0.0f,  // bottom-right
    };

    glGenVertexArrays(1, &this->VAO);
    glBindVertexArray(this->VAO);

    unsigned int VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(particle_quad), particle_quad, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);

    glGenBuffers(1, &this->instanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, this->instanceVBO);

    // GPT SUCCKS, GEMINI >>>>>
    glBufferData(GL_ARRAY_BUFFER, maxParticles * 8 * sizeof(float), NULL, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(4 * sizeof(float)));

    glVertexAttribDivisor(1, 1);
    glVertexAttribDivisor(2, 1);

    glBindVertexArray(0);
}

void ParticleEmitter::SpawnParticle(Particle particle)
{
    unsigned int particleIndex = this->firstUnusedParticle();
    if (particleIndex < this->particles.size())
    {
        Particle &p = this->particles[particleIndex];
        p.Position = particle.Position + Position;
        p.Velocity = particle.Velocity;
        p.Life = particle.Life;
        p.Size = particle.Size;
        p.Color = particle.Color;
    }
}

void ParticleEmitter::Update(float deltaTime)
{
    for (Particle &p : this->particles)
    {
        if (p.Life > 0.0f)
        {
            p.Life -= deltaTime;
            p.Position += p.Velocity * deltaTime;
        }
    }
}

void ParticleEmitter::Draw()
{
    int dataIndex = 0;
    int activeParticles = 0;

    for (const Particle &p : this->particles)
    {
        if (p.Life > 0.0f)
        {
            // Copy position and size (as w-component)
            this->particleData[dataIndex++] = p.Position.x;
            this->particleData[dataIndex++] = p.Position.y;
            this->particleData[dataIndex++] = p.Position.z;
            this->particleData[dataIndex++] = p.Size; // Use the particle's size

            // Copy color data
            this->particleData[dataIndex++] = p.Color.r;
            this->particleData[dataIndex++] = p.Color.g;
            this->particleData[dataIndex++] = p.Color.b;
            this->particleData[dataIndex++] = p.Color.a;

            activeParticles++;
        }
    }

    if (activeParticles > 0)
    {
        // Bind the instance VBO to update its data
        glBindBuffer(GL_ARRAY_BUFFER, this->instanceVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, activeParticles * 8 * sizeof(float), &this->particleData[0]);

        // Use the particle shader and bind the VAO
        this->shader.use();
        glBindVertexArray(this->VAO);

        // Enable blending for transparent particles
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Additive blending for fire/smoke

        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, activeParticles);

        glBindVertexArray(0);
        glDisable(GL_BLEND);
    }
}

unsigned int ParticleEmitter::firstUnusedParticle()
{
    for (unsigned int i = lastUsedParticle; i < this->particles.size(); i++)
    {
        if (this->particles[i].Life <= 0.0f)
        {
            lastUsedParticle = i;
            return i;
        }
    }

    for (unsigned int i = 0; i < lastUsedParticle; ++i)
    {
        if (this->particles[i].Life <= 0.0f)
        {
            lastUsedParticle = i;
            return i;
        }
    }

    lastUsedParticle = 0;
    return 0;
}
