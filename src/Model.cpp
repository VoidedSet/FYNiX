#include "Model.h"
#include "Mesh.h"
#include <glm/gtx/string_cast.hpp>

Model::Model(const std::string &path, unsigned int ID) : ID(ID), directory(path)
{
    if (loadModel(path))
    {
        std::cout << "[Model] Model loaded successfully from: " << path << std::endl;
    }
    else
    {
        std::cerr << "[Model] Failed to load model from: " << path << std::endl;
    }
}

void Model::Draw(Shader &shader)
{
    shader.use();
    shader.setUniforms("isAnimated", (unsigned int)UniformType::Bool, (void *)&hasAnimation);

    if (hasAnimation)
    {
        for (int i = 0; i < finalBoneMatrices.size(); i++)
        {
            std::string uniformName = "bone_transforms[" + std::to_string(i) + "]";
            shader.setUniforms(uniformName.c_str(), (unsigned int)UniformType::Mat4f, (void *)(glm::value_ptr(finalBoneMatrices[i])));
        }
    }

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

bool Model::loadModel(std::string path)
{
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

    if (scene == nullptr)
    {
        std::cerr << "[Model - ERROR] Error loading model: " << importer.GetErrorString() << std::endl;
        return false;
    }

    globalInverseTransform = assimpToGlmMatrix(scene->mRootNode->mTransformation);
    globalInverseTransform = glm::inverse(globalInverseTransform);

    processNode(scene->mRootNode, scene);

    if (scene->HasAnimations())
    {
        hasAnimation = true;
        animator.loadAnimation(scene);
        finalBoneMatrices.resize(skeleton.boneCount, glm::mat4(1.0f));
    }

    return true;
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
        vertex.postition = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);

        if (mesh->HasNormals())
        {
            vertex.normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
        }

        if (mesh->mTextureCoords[0])
        {
            vertex.texCoords = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
        }
        else
            vertex.texCoords = glm::vec2(0.0f, 0.0f);

        vertex.boneIds = glm::ivec4(0);
        vertex.boneWeights = glm::vec4(0.0f);

        vertices.push_back(vertex);
    }

    std::unordered_map<std::string, std::pair<int, glm::mat4>> boneInfo = {};
    std::vector<int> boneCounts;
    boneCounts.resize(vertices.size(), 0);

    if (mesh->HasBones())
    {
        skeleton.boneCount = mesh->mNumBones;

        for (unsigned int i = 0; i < skeleton.boneCount; i++)
        {
            aiBone *bone = mesh->mBones[i];
            glm::mat4 m = assimpToGlmMatrix(bone->mOffsetMatrix);
            boneInfo[bone->mName.C_Str()] = {i, m};

            for (unsigned int j = 0; j < bone->mNumWeights; j++)
            {
                unsigned int id = bone->mWeights[j].mVertexId;
                float weight = bone->mWeights[j].mWeight;
                boneCounts[id]++;
                switch (boneCounts[id])
                {
                case 1:
                    vertices[id].boneIds.x = i;
                    vertices[id].boneWeights.x = weight;
                    break;
                case 2:
                    vertices[id].boneIds.y = i;
                    vertices[id].boneWeights.y = weight;
                    break;
                case 3:
                    vertices[id].boneIds.z = i;
                    vertices[id].boneWeights.z = weight;
                    break;
                case 4:
                    vertices[id].boneIds.w = i;
                    vertices[id].boneWeights.w = weight;
                    break;
                default:
                    break;
                }
            }
        }
        readSkeleton(skeleton.rootBone, scene->mRootNode, boneInfo);
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }

    if (mesh->mMaterialIndex >= 0)
    {
        aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
        std::vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
        std::vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
    }

    return Mesh(vertices, indices, textures);
}

bool Model::readSkeleton(Bone &boneOutput, aiNode *node, std::unordered_map<std::string, std::pair<int, glm::mat4>> &boneInfoTable)
{
    std::string nodeName = node->mName.C_Str();

    if (boneInfoTable.find(nodeName) != boneInfoTable.end())
    {
        boneOutput.name = nodeName;
        boneOutput.id = boneInfoTable[nodeName].first;
        boneOutput.offset = boneInfoTable[nodeName].second;

        for (unsigned int i = 0; i < node->mNumChildren; i++)
        {
            Bone child;
            readSkeleton(child, node->mChildren[i], boneInfoTable);
            boneOutput.children.push_back(child);
        }
        return true;
    }
    else
    {
        for (unsigned int i = 0; i < node->mNumChildren; i++)
        {
            if (readSkeleton(boneOutput, node->mChildren[i], boneInfoTable))
            {
                return true;
            }
        }
    }
    return false;
}

std::vector<Texture> Model::loadMaterialTextures(aiMaterial *mat, aiTextureType type, std::string typeName)
{
    std::vector<Texture> textures;
    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
    {
        aiString str;
        mat->GetTexture(type, i, &str);
        std::string fullPath = std::string(str.C_Str());

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
            textures_loaded.push_back(texture);
        }
    }
    return textures;
}

void Model::UpdateAnimation(float deltaTime)
{
    if (hasAnimation)
    {
        animator.updateAnimation(deltaTime, skeleton, finalBoneMatrices, globalInverseTransform);
    }
}

void Model::seek(float time)
{
    if (hasAnimation)
    {
        animator.seek(time, skeleton, finalBoneMatrices, globalInverseTransform);
    }
}