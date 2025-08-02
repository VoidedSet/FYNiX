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

#include "Mesh.h"
#include "Shader.h"
#include "Animator.h"

struct Bone
{
    int id = 0; // position of the bone in final upload array
    std::string name = "";
    glm::mat4 offset = glm::mat4(1.0f);
    std::vector<Bone> children = {};
};

struct Skeleton
{
    Bone rootBone;
    // std::unordered_map<std::string, Bone *> boneMap;
    unsigned int boneCount = 0;
};

class Model
{
public:
    // this will act as a reference between the model and the scene graph nodes
    unsigned int ID;
    std::string directory;
    glm::mat4 globalInverseTransform;
    bool hasAnimation = false;

    Model(const std::string &path, unsigned int ID);

    void Draw(Shader &shader);
    void SetDirectory(const std::string &dir) { directory = dir; }
    void UpdateAnimation(float deltaTime);

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
    std::vector<Animation> animations;
    Animator animator;
    float currentTime = 0.f;
    std::vector<glm::mat4> finalBoneMatrices;

    glm::vec3 position = glm::vec3(1.f);
    glm::vec3 rotation = glm::vec3(1.f);
    glm::vec3 scale = glm::vec3(1.f);

    bool loadModel(std::string path);
    void processNode(aiNode *node, const aiScene *scene);
    Mesh processMesh(aiMesh *mesh, const aiScene *scene);

    void loadAnimation(const aiScene *scene);

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

    glm::vec3 assimpToGlmVec3(aiVector3D vec)
    {
        return glm::vec3(vec.x, vec.y, vec.z);
    }

    inline glm::quat assimpToGlmQuat(aiQuaternion quat)
    {
        glm::quat q;
        q.x = quat.x;
        q.y = quat.y;
        q.z = quat.z;
        q.w = quat.w;

        return q;
    }

    std::pair<unsigned int, float> getTimeFraction(std::vector<float> &times, float &dt);
    void getPose(Animation &animation, Bone &skeletion, float dt, std::vector<glm::mat4> &output, glm::mat4 &parentTransform);
};
