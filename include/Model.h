#pragma once

#include <vector>
#include <map>
#include <string>
#include <iostream>

#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/mesh.h>
#include <assimp/material.h>
#include <assimp/MathFunctions.h>

#include "Mesh.h"
#include "Shader.h"
#include "Bone.h"

struct BoneInfo
{
    int id;
    glm::mat4 offset;
};

class Model
{
public:
    // this will act as a reference between the model and the scene graph nodes
    unsigned int ID;
    std::string directory;
    int BoneCounter = 0;
    std::map<std::string, BoneInfo> BoneInfoMap;

    Model(const std::string &path, unsigned int ID);

    void Draw(Shader &shader);
    void SetDirectory(const std::string &dir) { directory = dir; }

    void setPosition(const glm::vec3 &pos) { position = pos; }
    void setRotation(const glm::vec3 &rot) { rotation = rot; }
    void setScale(const glm::vec3 &scl) { scale = scl; }

    glm::vec3 getPosition() const { return position; }
    glm::vec3 getRotation() const { return rotation; }
    glm::vec3 getScale() const { return scale; }

    glm::mat4 getModelMatrix() const;

private:
    // Model Data
    std::vector<Mesh> meshes;

    glm::vec3 position = glm::vec3(1.f);
    glm::vec3 rotation = glm::vec3(1.f);
    glm::vec3 scale = glm::vec3(1.f);

    std::vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, std::string typeName);
    std::vector<Texture> textures_loaded;

    bool loadModel(std::string path);
    void processNode(aiNode *node, const aiScene *scene);
    Mesh processMesh(aiMesh *mesh, const aiScene *scene);

    void setVertexBoneDataToDefault(Vertex &vertex);
    void setVertexBoneData(Vertex &vertex, int boneID, float weight);
    void extractBoneWeight(std::vector<Vertex> &vertices, aiMesh *mesh, const aiScene *scene);

    glm::mat4 ConvertMatrixToGLMFormat(const aiMatrix4x4 &from)
    {
        glm::mat4 to;

        to[0][0] = from.a1;
        to[1][0] = from.a2;
        to[2][0] = from.a3;
        to[3][0] = from.a4;
        to[0][1] = from.b1;
        to[1][1] = from.b2;
        to[2][1] = from.b3;
        to[3][1] = from.b4;
        to[0][2] = from.c1;
        to[1][2] = from.c2;
        to[2][2] = from.c3;
        to[3][2] = from.c4;
        to[0][3] = from.d1;
        to[1][3] = from.d2;
        to[2][3] = from.d3;
        to[3][3] = from.d4;

        return to;
    }
};
