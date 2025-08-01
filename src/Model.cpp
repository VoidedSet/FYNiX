#include "Model.h"

Model::Model(const std::string &path, unsigned int ID) : ID(ID), directory(path)
{
    if (loadModel(path)) // Not Model::loadModel
    {
        std::cout << "[Model] Model loaded successfully from: " << path << std::endl;
        // std::cout << "[Model] Number of meshes: " << meshes.size() << std::endl;
    }
    else
    {
        std::cerr << "[Model] Failed to load model from: " << path << std::endl;
    }
}

void Model::Draw(Shader &shader)
{
    for (unsigned int i = 0; i < meshes.size(); i++)
    {
        meshes[i].Draw(shader);
    }
}

glm::mat4 Model::getModelMatrix() const
{
    glm::mat4 model = glm::mat4(1.0f);

    model = glm::translate(model, this->position);
    model = glm::rotate(model, this->rotation.x, glm::vec3(1.f, 0.f, 0.f));
    model = glm::rotate(model, this->rotation.y, glm::vec3(0.f, 1.f, 0.f));
    model = glm::rotate(model, this->rotation.z, glm::vec3(0.f, 0.f, 1.f));
    model = glm::scale(model, this->scale);

    return model;
}

void Model::processNode(aiNode *node, const aiScene *scene)
{
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        unsigned int meshIndex = node->mMeshes[i];
        aiMesh *mesh = scene->mMeshes[meshIndex];
        meshes.push_back(processMesh(mesh, scene));
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        processNode(node->mChildren[i], scene);
    }
}

Mesh Model::processMesh(aiMesh *mesh, const aiScene *scene)
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;

    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        Vertex vertex;
        setVertexBoneDataToDefault(vertex);

        vertex.position = glm::vec3(
            mesh->mVertices[i].x,
            mesh->mVertices[i].y,
            mesh->mVertices[i].z);

        if (mesh->HasNormals())
        {
            vertex.normal = glm::vec3(
                mesh->mNormals[i].x,
                mesh->mNormals[i].y,
                mesh->mNormals[i].z);
        }

        if (mesh->mTextureCoords[0])
        {
            vertex.texCoords = glm::vec2(
                mesh->mTextureCoords[0][i].x,
                mesh->mTextureCoords[0][i].y);
        }
        else
            vertex.texCoords = glm::vec2(0.0f, 0.0f);

        vertices.push_back(vertex);
    }

    extractBoneWeight(vertices, mesh, scene);
    std::cout << "[DEBUG] Total bones in model: " << BoneCounter << std::endl;

    for (Vertex &v : vertices)
    {
        float total = 0.0f;
        for (int i = 0; i < MAX_BONE_WEIGHTS; i++)
            total += v.m_Weights[i];
        if (total > 0.0f)
        {
            for (int i = 0; i < MAX_BONE_WEIGHTS; i++)
                v.m_Weights[i] /= total;
        }
    }

    if (mesh->mMaterialIndex >= 0)
    {
        aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];

        // std::cout << "[Model] Loading textures for mesh: " << mesh->mName.C_Str() << std::endl;

        std::vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

        std::vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }

    return Mesh(vertices, indices, textures);
}

void Model::setVertexBoneDataToDefault(Vertex &vertex)
{
    for (int i = 0; i < MAX_BONE_WEIGHTS; i++)
    {
        vertex.m_BoneIDs[i] = -1;
        vertex.m_Weights[i] = 0.f;
    }
}

void Model::setVertexBoneData(Vertex &vertex, int boneID, float weight)
{
    for (int i = 0; i < MAX_BONE_WEIGHTS; i++)
    {
        if (vertex.m_BoneIDs[i] < 0)
        {
            vertex.m_Weights[i] = weight;
            vertex.m_BoneIDs[i] = boneID;
            break;
        }
    }
}

void Model::extractBoneWeight(std::vector<Vertex> &vertices, aiMesh *mesh, const aiScene *scene)
{
    for (int boneIndex = 0; boneIndex < mesh->mNumBones; boneIndex++)
    {
        int boneID = -1;
        std::string boneName = mesh->mBones[boneIndex]->mName.C_Str();

        if (BoneInfoMap.find(boneName) == BoneInfoMap.end())
        {
            BoneInfo newBoneInfo;
            newBoneInfo.id = BoneCounter;
            newBoneInfo.offset = ConvertMatrixToGLMFormat(mesh->mBones[boneIndex]->mOffsetMatrix);
            BoneInfoMap[boneName] = newBoneInfo; // <- THIS LINE WAS MISSING
            boneID = BoneCounter++;
        }
        else
            boneID = BoneInfoMap[boneName].id;
        if (boneID == -1)
        {
            std::cout << "[Models] Invalid BoneID" << std::endl;
            return;
        }

        auto weights = mesh->mBones[boneIndex]->mWeights;
        int numWeights = mesh->mBones[boneIndex]->mNumWeights;

        for (int weightIndex = 0; weightIndex < numWeights; weightIndex++)
        {
            int vertexID = weights[weightIndex].mVertexId;
            float weight = weights[weightIndex].mWeight;
            assert(vertexID <= vertices.size());
            setVertexBoneData(vertices[vertexID], boneID, weight);
        }
    }
}

std::vector<Texture> Model::loadMaterialTextures(aiMaterial *mat, aiTextureType type, std::string typeName)
{
    std::vector<Texture> textures;

    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
    {
        aiString str;
        mat->GetTexture(type, i, &str);
        std::string fullPath = std::string(str.C_Str()); // directory + "/" +

        bool skip = false;
        for (Texture &t : textures_loaded)
        {
            if (t.path == fullPath)
            {
                textures.push_back(t);
                skip = true;
                break;
            }
        }

        if (!skip)
        {
            Texture texture(fullPath.c_str(), GL_TEXTURE_2D, i, typeName);
            texture.path = fullPath;
            textures.push_back(texture);
            textures_loaded.push_back(texture); // add to cache
        }
        // std::cout << "[Texture] Loading: " << fullPath << " (type: " << typeName << ")" << std::endl;
    }

    return textures;
}

bool Model::loadModel(std::string path)
{
    Assimp::Importer importer;

    const aiScene *scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

    if (scene == nullptr)
    {
        std::cerr << "[Model - ERROR] Error loading model: " << importer.GetErrorString() << std::endl;
        return false;
    }

    if (scene->mAnimations && scene->mNumAnimations > 0)
    {
        std::cout << "[Model] Animation: " << scene->mAnimations[0]->mName.C_Str() << std::endl;
    }
    else
    {
        std::cout << "[Model] No animations found in model." << std::endl;
    }
    processNode(scene->mRootNode, scene);

    return true;
}