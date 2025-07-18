#pragma once

#include <vector>
#include <string>
#include <iostream>

#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/mesh.h>
#include <assimp/material.h>

#include "Mesh.h"
#include "Shader.h"

class Model
{
public:
    // this will act as a reference between the model and the scene graph nodes
    unsigned int ID;
    std::string directory;

    Model(const std::string &path, unsigned int ID);

    void Draw(Shader &shader);
    void SetDirectory(const std::string &dir) { directory = dir; }

private:
    // Model Data
    std::vector<Mesh> meshes;
    std::string dir;

    // Internal functions
    bool loadModel(std::string path);
    void processNode(aiNode *node, const aiScene *scene);
    Mesh processMesh(aiMesh *mesh, const aiScene *scene);

    std::vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, std::string typeName);
};
