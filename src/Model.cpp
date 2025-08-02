#include "Model.h"
#include "Mesh.h"
#include <glm/gtx/string_cast.hpp>

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
    shader.use();
    shader.setUniforms("isAnimated", (unsigned int)UniformType::Bool, (void *)&hasAnimation);

    if (hasAnimation)
    {
        for (int i = 0; i < skeleton.boneCount; i++) // use actual bone count
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

// a recursive function to read all bones and form skeleton
bool Model::readSkeleton(Bone &boneOutput, aiNode *node, std::unordered_map<std::string, std::pair<int, glm::mat4>> &boneInfoTable)
{
    std::string nodeName = node->mName.C_Str();
    if (nodeName.empty())
    {
        std::cout << "[Skeleton] Skipping unnamed node" << std::endl;
        return false;
    }

    auto it = boneInfoTable.find(nodeName);
    if (it != boneInfoTable.end())
    {
        boneOutput.name = nodeName;
        boneOutput.id = it->second.first;
        boneOutput.offset = it->second.second;

        std::cout << "[Skeleton] Reading bone: '" << boneOutput.name << "' (id=" << boneOutput.id << ")" << std::endl;

        for (unsigned int i = 0; i < node->mNumChildren; i++)
        {
            Bone child;
            if (readSkeleton(child, node->mChildren[i], boneInfoTable))
            {
                boneOutput.children.push_back(child);
            }
        }

        return true;
    }
    else
    {
        // Recurse into children in case the actual bone is nested deeper
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

Mesh Model::processMesh(aiMesh *mesh, const aiScene *scene)
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;

    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        Vertex vertex;

        vertex.postition = glm::vec3(
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

        vertex.boneIds = glm::ivec4(0.f);
        vertex.boneWeights = glm::vec4(0.f);

        vertices.push_back(vertex);
    }

    // load boneData to vertices
    std::unordered_map<std::string, std::pair<int, glm::mat4>> boneInfo = {};
    std::vector<unsigned int> boneCounts;
    boneCounts.resize(vertices.size(), 0);
    skeleton.boneCount = mesh->mNumBones;

    for (unsigned int i = 0; i < skeleton.boneCount; i++)
    {
        aiBone *bone = mesh->mBones[i];
        glm::mat4 m = assimpToGlmMatrix(bone->mOffsetMatrix);
        boneInfo[bone->mName.C_Str()] = {i, m};

        // loop through each vertex that have that bone
        for (int j = 0; j < bone->mNumWeights; j++)
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
                std::cout << "err: unable to allocate bone to vertex" << std::endl;
                break;
            }
        }
    }

    // normalize weights to make all weights sum 1
    for (int i = 0; i < vertices.size(); i++)
    {
        glm::vec4 &boneWeights = vertices[i].boneWeights;
        float totalWeight = boneWeights.x + boneWeights.y + boneWeights.z + boneWeights.w;
        if (totalWeight > 0.0f)
        {
            vertices[i].boneWeights = glm::vec4(
                boneWeights.x / totalWeight,
                boneWeights.y / totalWeight,
                boneWeights.z / totalWeight,
                boneWeights.w / totalWeight);
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
    bool rootFound = false;
    for (unsigned int i = 0; i < scene->mRootNode->mNumChildren && !rootFound; ++i)
    {
        rootFound = readSkeleton(skeleton.rootBone, scene->mRootNode->mChildren[i], boneInfo);
    }

    if (!rootFound)
    {
        std::cerr << "[Skeleton] Failed to build skeleton: no matching bone node found!" << std::endl;
    }
    else
    {
        finalBoneMatrices.resize(skeleton.boneCount, glm::mat4(1.0f));
    }

    std::cout << "[Model] Mesh " << mesh->mName.C_Str() << " has " << skeleton.boneCount << " bones!" << std::endl;
    return Mesh(vertices, indices, textures);
}

void Model::loadAnimation(const aiScene *scene)
{
    if (!scene || scene->mNumAnimations < 1)
    {
        hasAnimation = false;
        return;
    }

    hasAnimation = true;
    // loading  first Animation
    aiAnimation *anim = scene->mAnimations[0];
    Animation animation;

    if (anim->mTicksPerSecond != 0.0f)
        animation.ticksPerSecond = anim->mTicksPerSecond;
    else
        animation.ticksPerSecond = 1;

    animation.duration = anim->mDuration * anim->mTicksPerSecond;
    animation.boneTransforms = {};

    // load positions rotations and scales for each bone
    //  each channel represents each bone
    for (int i = 0; i < anim->mNumChannels; i++)
    {
        aiNodeAnim *channel = anim->mChannels[i];
        BoneTransformTrack track;
        for (int j = 0; j < channel->mNumPositionKeys; j++)
        {
            track.positionTimestamps.push_back(channel->mPositionKeys[j].mTime);
            track.positions.push_back(assimpToGlmVec3(channel->mPositionKeys[j].mValue));
        }
        for (int j = 0; j < channel->mNumRotationKeys; j++)
        {
            track.rotationTimestamps.push_back(channel->mRotationKeys[j].mTime);
            track.rotations.push_back(assimpToGlmQuat(channel->mRotationKeys[j].mValue));
        }
        for (int j = 0; j < channel->mNumScalingKeys; j++)
        {
            track.scaleTimestamps.push_back(channel->mScalingKeys[j].mTime);
            track.scales.push_back(assimpToGlmVec3(channel->mScalingKeys[j].mValue));
        }
        animation.boneTransforms[channel->mNodeName.C_Str()] = track;
    }

    animations.push_back(animation);
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

    globalInverseTransform = assimpToGlmMatrix(scene->mRootNode->mTransformation);
    globalInverseTransform = glm::inverse(globalInverseTransform);

    processNode(scene->mRootNode, scene);
    loadAnimation(scene);
    finalBoneMatrices.clear();
    finalBoneMatrices.resize(skeleton.boneCount, glm::mat4(1.0f));

    return true;
}

std::pair<unsigned int, float> Model::getTimeFraction(std::vector<float> &times, float &dt)
{
    if (times.size() < 2)
        return {0, 0.0f}; // avoid divide by zero

    unsigned int segment = 1;
    while (segment < times.size() && dt > times[segment])
        segment++;

    if (segment >= times.size())
        segment = times.size() - 1;

    float start = times[segment - 1];
    float end = times[segment];
    float frac = (dt - start) / (end - start);
    return {segment, frac};
}

void Model::getPose(Animation &animation, Bone &skeletion, float dt, std::vector<glm::mat4> &output, glm::mat4 &parentTransform)
{
    if (animation.boneTransforms.find(skeletion.name) == animation.boneTransforms.end())
    {
        // std::cout << "[WARN] No animation track for bone: " << skeletion.name << std::endl;
        return;
    }

    BoneTransformTrack &btt = animation.boneTransforms[skeletion.name];
    dt = fmod(dt, animation.duration);
    std::pair<unsigned int, float> fp;
    // calculate interpolated position
    if (btt.positionTimestamps.empty() || btt.rotationTimestamps.empty() || btt.scaleTimestamps.empty())
    {
        std::cout << "[WARN] Bone '" << skeletion.name << "' has missing keyframes" << std::endl;
        return;
    }

    fp = getTimeFraction(btt.positionTimestamps, dt);

    glm::vec3 position1 = btt.positions[fp.first - 1];
    glm::vec3 position2 = btt.positions[fp.first];

    glm::vec3 position = glm::mix(position1, position2, fp.second);

    // calculate interpolated rotation
    fp = getTimeFraction(btt.rotationTimestamps, dt);
    glm::quat rotation1 = btt.rotations[fp.first - 1];
    glm::quat rotation2 = btt.rotations[fp.first];

    glm::quat rotation = glm::slerp(rotation1, rotation2, fp.second);

    // calculate interpolated scale
    fp = getTimeFraction(btt.scaleTimestamps, dt);
    glm::vec3 scale1 = btt.scales[fp.first - 1];
    glm::vec3 scale2 = btt.scales[fp.first];

    glm::vec3 scale = glm::mix(scale1, scale2, fp.second);

    glm::mat4 positionMat = glm::mat4(1.0),
              scaleMat = glm::mat4(1.0);

    // calculate localTransform
    positionMat = glm::translate(positionMat, position);
    glm::mat4 rotationMat = glm::toMat4(rotation);
    scaleMat = glm::scale(scaleMat, scale);
    glm::mat4 localTransform = positionMat * rotationMat * scaleMat;
    glm::mat4 globalTransform = parentTransform * localTransform;

    output[skeletion.id] = globalInverseTransform * globalTransform * skeletion.offset;
    // update values for children bones
    for (Bone &child : skeletion.children)
    {
        getPose(animation, child, dt, output, globalTransform);
    }
    // std::cout << dt << " => " << position.x << ":" << position.y << ":" << position.z << ":" << std::endl;
}
void Model::UpdateAnimation(float deltaTime)
{
    if (!hasAnimation || animations.empty())
        return;

    // std::cout << "Animation update t=" << currentTime << std::endl;

    for (int i = 0; i < skeleton.boneCount; ++i)
    {
        const glm::mat4 &m = finalBoneMatrices[i];
        // std::cout << "Bone[" << i << "] = " << glm::to_string(m[3]) << std::endl; // print translation
    }

    currentTime += deltaTime;

    if (finalBoneMatrices.size() != skeleton.boneCount)
        finalBoneMatrices.resize(skeleton.boneCount, glm::mat4(1.0f)); // 💥 this was missing

    std::vector<glm::mat4> boneMatrices(skeleton.boneCount);
    glm::mat4 identity = glm::mat4(1.0f);

    getPose(animations[0], skeleton.rootBone, currentTime, boneMatrices, identity);

    for (int i = 0; i < boneMatrices.size(); i++)
        finalBoneMatrices[i] = boneMatrices[i];
}
