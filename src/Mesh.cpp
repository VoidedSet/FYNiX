#include "Mesh.h"

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

    glEnableVertexAttribArray(3);
    glVertexAttribIPointer(3, 4, GL_INT, sizeof(Vertex), (GLvoid *)offsetof(Vertex, boneIds));

    VAO.AddAttribLayout(4, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)offsetof(Vertex, boneWeights)); // bone wieghts

    VAO.UnBind();
    VBO.UnBind();
    EBO.UnBind();

    // std::cout << "[Mesh] Texture count : " << textures.size() << std::endl;
}

Mesh::Mesh(MeshType type) : VBO(VertexBuffer(cubeVert, sizeof(cubeVert))), EBO(ElementBuffer(this->cubeIndices, sizeof(this->cubeIndices)))
{
    indices.push_back(0);
    if (type == MeshType::CUBE)
    {
        VAO = VertexArray();
        VAO.Bind();
        VBO.Bind();
        EBO.Bind();

        VAO.AddAttribLayout(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);                   // position
        VAO.AddAttribLayout(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3 * sizeof(float))); // texCoords

        VAO.UnBind();
        VBO.UnBind();
        EBO.UnBind();
    }
}

void Mesh::Draw(Shader &shader)
{
    // bind textures, and will also update required shader uniforms.
    shader.use();

    for (unsigned int i = 0; i < textures.size(); i++)
    {
        textures[i].Bind(i);
        textures[i].SetUniform(shader, textures[i].type + std::to_string(i));
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