#pragma once

#include <vector>
#include <iostream>
#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Texture.h"
#include "Shader.h"
#include "BufferObjects/ElementBuffer.h"
#include "BufferObjects/VertexArray.h"
#include "BufferObjects/VertexBuffer.h"

#define MAX_BONE_INFLUENCE 4
#define MAX_BONE_WEIGHTS 4

struct Vertex
{
    glm::vec3 position, normal;
    glm::vec2 texCoords;

    int m_BoneIDs[MAX_BONE_INFLUENCE];
    float m_Weights[MAX_BONE_INFLUENCE];
};

enum MeshType
{
    CUBE,
    SPHERE,
    PLANE,
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

    Mesh(std::vector<Vertex> vert, std::vector<unsigned int> inds, std::vector<Texture> texs);

    Mesh(MeshType type);

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
