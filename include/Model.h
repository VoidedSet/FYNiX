#pragma once

#include <vector>
#include <string>
#include <iostream>
#include <json.hpp>

#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/mesh.h>
#include <assimp/material.h>

#include "Mesh.h"
#include "Shader.h"
#include "Animation.h"
#include "Animator.h"

class Model
{
public:
    // this will act as a reference between the model and the scene graph nodes
    unsigned int ID;
    std::string directory;
    bool isReady = false;

    Model(const std::string &path, unsigned int ID);

    void Update(float deltaTime);
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

    bool hasAnimations = false;
    std::vector<Animation> animations;
    std::vector<Animator> animators;

    bool loadModel(std::string path);
    void loadAnimationsFromGLTF(const std::string &gltfPath);
    void processNode(aiNode *node, const aiScene *scene);
    Mesh processMesh(aiMesh *mesh, const aiScene *scene);

    std::vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, std::string typeName);
    std::vector<Texture> textures_loaded;
};
