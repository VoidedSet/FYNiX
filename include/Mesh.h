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

struct Vertex
{
    glm::vec3 postition, normal;
    glm::vec2 texCoords;
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

    void Draw(Shader &shader);
};