#pragma once

#include <vector>
#include <string>
#include <iostream>

#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/mesh.h>
#include <assimp/material.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

// (Include other necessary headers: vector, string, glm, etc.)
#include "Mesh.h"
#include "Shader.h"
#include "Animator.h"

// Assuming Bone and Skeleton are defined here or included from another file
struct Bone
{
    int id = 0;
    std::string name = "";
    glm::mat4 offset = glm::mat4(1.0f);
    std::vector<Bone> children = {};
};

struct Skeleton
{
    Bone rootBone;
    unsigned int boneCount = 0;
};

class Model
{
public:
    unsigned int ID;
    std::string directory;
    glm::mat4 globalInverseTransform;
    bool hasAnimation = false;

    Model(const std::string &path, unsigned int ID);

    void Draw(Shader &shader);
    void UpdateAnimation(float deltaTime);

    // --- Animation Control Wrappers ---
    void seek(float time);
    Animator &getAnimator() { return animator; }

    // (rest of your public methods: setPosition, etc.)
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
    Skeleton skeleton;
    Animator animator; // Now accessed via getAnimator()
    std::vector<glm::mat4> finalBoneMatrices;

    // (private members and methods)
    glm::vec3 position = glm::vec3(1.f);
    glm::vec3 rotation = glm::vec3(1.f);
    glm::vec3 scale = glm::vec3(1.f);

    bool loadModel(std::string path);
    void processNode(aiNode *node, const aiScene *scene);
    Mesh processMesh(aiMesh *mesh, const aiScene *scene);

    bool readSkeleton(Bone &boneOutput, aiNode *node, std::unordered_map<std::string, std::pair<int, glm::mat4>> &boneInfoTable);

    std::vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, std::string typeName);
    std::vector<Texture> textures_loaded;

    glm::mat4 assimpToGlmMatrix(aiMatrix4x4 mat)
    {
        glm::mat4 m;
        for (int y = 0; y < 4; y++)
        {
            for (int x = 0; x < 4; x++)
            {
                m[x][y] = mat[y][x];
            }
        }
        return m;
    }
};