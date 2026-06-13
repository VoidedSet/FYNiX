#pragma once

#include <vector>
#include <string>

#include <glm/glm.hpp>

#include "Texture.h"
#include "Shader.h"
#include "BufferObjects/ElementBuffer.h"
#include "BufferObjects/VertexArray.h"
#include "BufferObjects/VertexBuffer.h"

struct Vertex
{
    glm::vec3 postition, normal;
    glm::vec2 texCoords;

    glm::ivec4 boneIds = glm::ivec4(0);
    glm::vec4 boneWeights = glm::vec4(0.f);
};

enum MeshType
{
    CUBE,
    SPHERE,
    PLANE,
    CYLINDER,
    CONE,
    CAMERA_PYRAMID
};

class Mesh
{
public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;

    VertexArray VAO;
    VertexBuffer VBO;
    ElementBuffer EBO;

    Mesh(std::vector<Vertex> vert, std::vector<unsigned int> inds, std::vector<Texture> texs, bool uploadToGPU = true);
    Mesh(MeshType type);

    void UploadToGPU();
    void Draw(Shader &shader);

private:
    unsigned int cubeIndices[36] = {
        0, 1, 2, 2, 3, 0, // front
        4, 5, 6, 6, 7, 4, // back
        4, 5, 1, 1, 0, 4, // bottom
        6, 7, 3, 3, 2, 6, // top
        4, 7, 3, 3, 0, 4, // left
        1, 5, 6, 6, 2, 1  // right
    };
};
