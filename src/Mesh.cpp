#include "Mesh.h"

Mesh::Mesh(std::vector<Vertex> vert, std::vector<unsigned int> inds, std::vector<Texture> texs)
    : vertices(vert), indices(inds), textures(texs),
      VBO(vertices.data(), vertices.size() * sizeof(Vertex)),
      EBO(indices.data(), indices.size() * sizeof(unsigned int))
{
    VAO = VertexArray();
    VAO.Bind();
    VBO.Bind();
    EBO.Bind();

    VAO.AddAttribLayout(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0);                           // position
    VAO.AddAttribLayout(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, normal));    // normal
    VAO.AddAttribLayout(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, texCoords)); // texCoords

    VAO.UnBind();
    VBO.UnBind();
    EBO.UnBind();
}

void Mesh::Draw(Shader &shader)
{
    // bind textures, and will also update required shader uniforms.

    VAO.Bind();
    VBO.Bind();
    EBO.Bind();
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    VAO.UnBind();
    VBO.UnBind();
    EBO.UnBind();
}