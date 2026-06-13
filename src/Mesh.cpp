#include "Mesh.h"
#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace
{
    const std::vector<std::string> diffuseUniformNames = []() {
        std::vector<std::string> names;
        for (int i = 0; i < 16; ++i)
        {
            names.push_back("texture_diffuse" + std::to_string(i));
        }
        return names;
    }();

    const std::vector<std::string> specularUniformNames = []() {
        std::vector<std::string> names;
        for (int i = 0; i < 16; ++i)
        {
            names.push_back("texture_specular" + std::to_string(i));
        }
        return names;
    }();
}

float cubeVert[] = {
    -0.5f, -0.5f, -0.5f, 0.0f, 0.0f,
    0.5f, -0.5f, -0.5f, 1.0f, 0.0f,
    0.5f, 0.5f, -0.5f, 1.0f, 1.0f,
    0.5f, 0.5f, -0.5f, 1.0f, 1.0f,
    -0.5f, 0.5f, -0.5f, 0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f, 0.0f, 0.0f,
    -0.5f, -0.5f, 0.5f, 0.0f, 0.0f,
    0.5f, -0.5f, 0.5f, 1.0f, 0.0f,
    0.5f, 0.5f, 0.5f, 1.0f, 1.0f,
    0.5f, 0.5f, 0.5f, 1.0f, 1.0f,
    -0.5f, 0.5f, 0.5f, 0.0f, 1.0f,
    -0.5f, -0.5f, 0.5f, 0.0f, 0.0f,
    -0.5f, 0.5f, 0.5f, 1.0f, 0.0f,
    -0.5f, 0.5f, -0.5f, 1.0f, 1.0f,
    -0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
    -0.5f, -0.5f, 0.5f, 0.0f, 0.0f,
    -0.5f, 0.5f, 0.5f, 1.0f, 0.0f,
    0.5f, 0.5f, 0.5f, 1.0f, 0.0f,
    0.5f, 0.5f, -0.5f, 1.0f, 1.0f,
    0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
    0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
    0.5f, -0.5f, 0.5f, 0.0f, 0.0f,
    0.5f, 0.5f, 0.5f, 1.0f, 0.0f,
    -0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
    0.5f, -0.5f, -0.5f, 1.0f, 1.0f,
    0.5f, -0.5f, 0.5f, 1.0f, 0.0f,
    0.5f, -0.5f, 0.5f, 1.0f, 0.0f,
    -0.5f, -0.5f, 0.5f, 0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
    -0.5f, 0.5f, -0.5f, 0.0f, 1.0f,
    0.5f, 0.5f, -0.5f, 1.0f, 1.0f,
    0.5f, 0.5f, 0.5f, 1.0f, 0.0f,
    0.5f, 0.5f, 0.5f, 1.0f, 0.0f,
    -0.5f, 0.5f, 0.5f, 0.0f, 0.0f,
    -0.5f, 0.5f, -0.5f, 0.0f, 1.0f};

Mesh::Mesh(std::vector<Vertex> vert, std::vector<unsigned int> inds, std::vector<Texture> texs, bool uploadToGPU)
    : vertices(vert), indices(inds), textures(texs)
{
    if (uploadToGPU)
    {
        UploadToGPU();
    }
}

void Mesh::UploadToGPU()
{
    VBO.Init(vertices.data(), vertices.size() * sizeof(Vertex));
    EBO.Init(indices.data(), indices.size() * sizeof(unsigned int));

    VAO.Init();
    VAO.Bind();
    VBO.Bind();
    EBO.Bind();

    VAO.AddAttribLayout(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0);                           // position
    VAO.AddAttribLayout(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, normal));    // normal
    VAO.AddAttribLayout(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, texCoords)); // texCoords

    glEnableVertexAttribArray(3);
    glVertexAttribIPointer(3, 4, GL_INT, sizeof(Vertex), (GLvoid *)offsetof(Vertex, boneIds));

    VAO.AddAttribLayout(4, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)offsetof(Vertex, boneWeights)); // bone weights

    VAO.UnBind();
    VBO.UnBind();
    EBO.UnBind();
}

Mesh::Mesh(MeshType type)
{
    if (type == MeshType::CUBE)
    {
        // Front face (normal 0, 0, 1)
        vertices.push_back({{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}});
        vertices.push_back({{ 0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}});
        vertices.push_back({{ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}});
        vertices.push_back({{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}});

        // Back face (normal 0, 0, -1)
        vertices.push_back({{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}});
        vertices.push_back({{-0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}});
        vertices.push_back({{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}});
        vertices.push_back({{ 0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}});

        // Left face (normal -1, 0, 0)
        vertices.push_back({{-0.5f,  0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}});
        vertices.push_back({{-0.5f,  0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}});
        vertices.push_back({{-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}});
        vertices.push_back({{-0.5f, -0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}});

        // Right face (normal 1, 0, 0)
        vertices.push_back({{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}});
        vertices.push_back({{ 0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}});
        vertices.push_back({{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}});
        vertices.push_back({{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}});

        // Top face (normal 0, 1, 0)
        vertices.push_back({{-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}});
        vertices.push_back({{-0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}});
        vertices.push_back({{ 0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}});
        vertices.push_back({{ 0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}});

        // Bottom face (normal 0, -1, 0)
        vertices.push_back({{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}});
        vertices.push_back({{ 0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}});
        vertices.push_back({{ 0.5f, -0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}});
        vertices.push_back({{-0.5f, -0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}});

        for (int i = 0; i < 6; ++i)
        {
            int start = i * 4;
            indices.push_back(start);
            indices.push_back(start + 1);
            indices.push_back(start + 2);
            indices.push_back(start);
            indices.push_back(start + 2);
            indices.push_back(start + 3);
        }
    }
    else if (type == MeshType::SPHERE)
    {
        const unsigned int X_SEGMENTS = 16;
        const unsigned int Y_SEGMENTS = 16;
        const float PI = 3.14159265359f;

        for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
        {
            for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
            {
                float xSegment = (float)x / (float)X_SEGMENTS;
                float ySegment = (float)y / (float)Y_SEGMENTS;
                float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
                float yPos = std::cos(ySegment * PI);
                float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

                glm::vec3 pos(xPos * 0.5f, yPos * 0.5f, zPos * 0.5f);
                glm::vec3 normal(xPos, yPos, zPos);
                glm::vec2 uv(xSegment, ySegment);

                vertices.push_back({pos, normal, uv});
            }
        }

        for (unsigned int y = 0; y < Y_SEGMENTS; ++y)
        {
            for (unsigned int x = 0; x < X_SEGMENTS; ++x)
            {
                indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                indices.push_back(y * (X_SEGMENTS + 1) + x);
                indices.push_back(y * (X_SEGMENTS + 1) + x + 1);

                indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                indices.push_back(y * (X_SEGMENTS + 1) + x + 1);
                indices.push_back((y + 1) * (X_SEGMENTS + 1) + x + 1);
            }
        }
    }
    else if (type == MeshType::PLANE)
    {
        vertices.push_back({{-0.5f, 0.0f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}});
        vertices.push_back({{-0.5f, 0.0f,  0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}});
        vertices.push_back({{ 0.5f, 0.0f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}});
        vertices.push_back({{ 0.5f, 0.0f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}});

        indices.push_back(0);
        indices.push_back(1);
        indices.push_back(2);
        indices.push_back(0);
        indices.push_back(2);
        indices.push_back(3);
    }
    else if (type == MeshType::CYLINDER)
    {
        const unsigned int SEGMENTS = 16;
        const float PI = 3.14159265359f;
        float height = 1.0f;
        float radius = 0.5f;

        // Side vertices
        for (unsigned int i = 0; i <= SEGMENTS; ++i)
        {
            float ratio = (float)i / (float)SEGMENTS;
            float angle = ratio * 2.0f * PI;
            float x = std::cos(angle);
            float z = std::sin(angle);

            vertices.push_back({{x * radius, -height * 0.5f, z * radius}, {x, 0.0f, z}, {ratio, 0.0f}});
            vertices.push_back({{x * radius, height * 0.5f, z * radius}, {x, 0.0f, z}, {ratio, 1.0f}});
        }

        // Side indices
        for (unsigned int i = 0; i < SEGMENTS; ++i)
        {
            unsigned int b1 = i * 2;
            unsigned int t1 = i * 2 + 1;
            unsigned int b2 = (i + 1) * 2;
            unsigned int t2 = (i + 1) * 2 + 1;

            indices.push_back(b1);
            indices.push_back(t1);
            indices.push_back(t2);

            indices.push_back(b1);
            indices.push_back(t2);
            indices.push_back(b2);
        }

        // Top cap
        unsigned int topCapStartIndex = vertices.size();
        vertices.push_back({{0.0f, height * 0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.5f, 0.5f}});
        for (unsigned int i = 0; i <= SEGMENTS; ++i)
        {
            float ratio = (float)i / (float)SEGMENTS;
            float angle = ratio * 2.0f * PI;
            float x = std::cos(angle);
            float z = std::sin(angle);
            vertices.push_back({{x * radius, height * 0.5f, z * radius}, {0.0f, 1.0f, 0.0f}, {x * 0.5f + 0.5f, z * 0.5f + 0.5f}});
        }
        for (unsigned int i = 0; i < SEGMENTS; ++i)
        {
            indices.push_back(topCapStartIndex);
            indices.push_back(topCapStartIndex + 1 + i + 1);
            indices.push_back(topCapStartIndex + 1 + i);
        }

        // Bottom cap
        unsigned int bottomCapStartIndex = vertices.size();
        vertices.push_back({{0.0f, -height * 0.5f, 0.0f}, {0.0f, -1.0f, 0.0f}, {0.5f, 0.5f}});
        for (unsigned int i = 0; i <= SEGMENTS; ++i)
        {
            float ratio = (float)i / (float)SEGMENTS;
            float angle = ratio * 2.0f * PI;
            float x = std::cos(angle);
            float z = std::sin(angle);
            vertices.push_back({{x * radius, -height * 0.5f, z * radius}, {0.0f, -1.0f, 0.0f}, {x * 0.5f + 0.5f, z * 0.5f + 0.5f}});
        }
        for (unsigned int i = 0; i < SEGMENTS; ++i)
        {
            indices.push_back(bottomCapStartIndex);
            indices.push_back(bottomCapStartIndex + 1 + i);
            indices.push_back(bottomCapStartIndex + 1 + i + 1);
        }
    }
    else if (type == MeshType::CONE)
    {
        const unsigned int SEGMENTS = 16;
        const float PI = 3.14159265359f;
        float height = 1.0f;
        float radius = 0.5f;

        // Side vertices pointing along -Z (pointing forward for spot light!)
        for (unsigned int i = 0; i <= SEGMENTS; ++i)
        {
            float ratio = (float)i / (float)SEGMENTS;
            float angle = ratio * 2.0f * PI;
            float x = std::cos(angle);
            float y = std::sin(angle);

            float len = std::sqrt(radius * radius + height * height);
            float nz = radius / len;
            float nxy = height / len;

            // Base vertex
            vertices.push_back({{x * radius, y * radius, -height}, {x * nxy, y * nxy, nz}, {ratio, 0.0f}});
            // Apex vertex
            vertices.push_back({{0.0f, 0.0f, 0.0f}, {x * nxy, y * nxy, nz}, {ratio, 1.0f}});
        }

        for (unsigned int i = 0; i < SEGMENTS; ++i)
        {
            // Reverse winding order to maintain counter-clockwise front face
            indices.push_back(i * 2);
            indices.push_back((i + 1) * 2);
            indices.push_back(i * 2 + 1);
        }

        // Base cap (normal pointing along -Z)
        unsigned int baseCapStartIndex = vertices.size();
        vertices.push_back({{0.0f, 0.0f, -height}, {0.0f, 0.0f, -1.0f}, {0.5f, 0.5f}});
        for (unsigned int i = 0; i <= SEGMENTS; ++i)
        {
            float ratio = (float)i / (float)SEGMENTS;
            float angle = ratio * 2.0f * PI;
            float x = std::cos(angle);
            float y = std::sin(angle);
            vertices.push_back({{x * radius, y * radius, -height}, {0.0f, 0.0f, -1.0f}, {x * 0.5f + 0.5f, y * 0.5f + 0.5f}});
        }
        for (unsigned int i = 0; i < SEGMENTS; ++i)
        {
            // Reverse winding order to maintain counter-clockwise front face
            indices.push_back(baseCapStartIndex);
            indices.push_back(baseCapStartIndex + 1 + i + 1);
            indices.push_back(baseCapStartIndex + 1 + i);
        }
    }
    else if (type == MeshType::CAMERA_PYRAMID)
    {
        // Apex at 0,0,0
        vertices.push_back({{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.5f, 0.5f}});
        
        // Base rectangle at z = -0.6f (looking down -Z)
        vertices.push_back({{-0.4f,  0.3f, -0.6f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}}); // TL
        vertices.push_back({{ 0.4f,  0.3f, -0.6f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}}); // TR
        vertices.push_back({{ 0.4f, -0.3f, -0.6f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}}); // BR
        vertices.push_back({{-0.4f, -0.3f, -0.6f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}}); // BL

        // Top cap triangle apex to indicate up
        vertices.push_back({{0.0f, 0.5f, -0.6f}, {0.0f, 0.0f, -1.0f}, {0.5f, 1.0f}}); // Top Dir

        // Sides
        indices.push_back(0); indices.push_back(1); indices.push_back(2);
        indices.push_back(0); indices.push_back(2); indices.push_back(3);
        indices.push_back(0); indices.push_back(3); indices.push_back(4);
        indices.push_back(0); indices.push_back(4); indices.push_back(1);

        // Base
        indices.push_back(1); indices.push_back(3); indices.push_back(2);
        indices.push_back(1); indices.push_back(4); indices.push_back(3);

        // Top triangle
        indices.push_back(1); indices.push_back(5); indices.push_back(2);
    }

    UploadToGPU();
}

void Mesh::Draw(Shader &shader)
{
    // bind textures, and will also update required shader uniforms.
    shader.use();

    bool hasDiffuseMap = false;
    bool hasSpecularMap = false;
    for (unsigned int i = 0; i < textures.size(); i++)
    {
        textures[i].Bind(i);
        std::string uniformName;
        if (textures[i].type == "texture_diffuse" && i < 16)
        {
            uniformName = diffuseUniformNames[i];
            hasDiffuseMap = true;
        }
        else if (textures[i].type == "texture_specular" && i < 16)
        {
            uniformName = specularUniformNames[i];
            hasSpecularMap = true;
        }
        else
        {
            uniformName = textures[i].type + std::to_string(i);
        }
        textures[i].SetUniform(shader, uniformName, i);
    }
    int useDiffuse = hasDiffuseMap ? 1 : 0;
    if (shader.getUniformLocation("useDiffuseMap") != -1)
    {
        shader.setUniforms("useDiffuseMap", static_cast<unsigned int>(UniformType::Int), &useDiffuse);
    }
    int useSpec = hasSpecularMap ? 1 : 0;
    if (shader.getUniformLocation("useSpecularMap") != -1)
    {
        shader.setUniforms("useSpecularMap", static_cast<unsigned int>(UniformType::Int), &useSpec);
    }

    VAO.Bind();
    VBO.Bind();
    EBO.Bind();
    if (indices.size() > 1)
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    else
    {
        glDrawArrays(GL_TRIANGLES, 0, 36);
        // std::cout << "[Mesh] Warning drawing without indexing. Vertex redundancy!" << std::endl;
    }
    VAO.UnBind();
    VBO.UnBind();
    EBO.UnBind();

    for (unsigned int i = 0; i < textures.size(); i++)
        textures[i].UnBind();
}