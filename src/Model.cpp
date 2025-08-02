#include "Model.h"

bool endsWith(const std::string &str, const std::string &suffix)
{
    return str.size() >= suffix.size() &&
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

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

    isReady = true;
}

void Model::Update(float deltaTime)
{
    // assert(animations.size() == animators.size() && "Animator-Animation mismatch!");

    if (animations.size() != animators.size())
    {
        std::cerr << "[Model::Update] Animation mismatch: " << animations.size() << " animations vs "
                  << animators.size() << " animators for model ID " << ID << std::endl;
        return;
    }

    for (auto &animator : animators)
        animator.Update(deltaTime);
}

void Model::Draw(Shader &shader)
{
    for (size_t i = 0; i < meshes.size(); ++i)
    {
        const std::string &meshName = meshes[i].name;
        bool drawn = false;

        for (size_t j = 0; j < animations.size(); ++j)
        {
            if (animations[j].targetMeshName == meshName)
            {
                const Animation &anim = animations[j];
                const Animator &animator = animators[j];

                float blend = animator.getBlendFactor();
                float time = animator.getTime();

                const std::vector<MorphFrame> &frames = anim.keyFrames;

                if (frames.size() < 2)
                    break;

                // Find current and next frame indices
                int currentFrame = 0;
                for (int k = 0; k < frames.size() - 1; ++k)
                {
                    if (time >= frames[k].time && time <= frames[k + 1].time)
                    {
                        currentFrame = k;
                        break;
                    }
                }

                int morphA = currentFrame;
                int morphB = std::min(currentFrame + 1, (int)frames.size() - 1);

                // Draw with morph target VBOs
                meshes[i].Draw(shader, morphA, morphB, blend);
                drawn = true;
                break;
            }
        }

        if (!drawn)
            meshes[i].Draw(shader); // fallback
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

        vertices.push_back(vertex);
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

    return Mesh(mesh->mName.C_Str(), vertices, indices, textures);
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

    // Only load animations if it's a .gltf file
    if (endsWith(path, ".gltf"))
    {
        std::ifstream in(path);
        if (in)
        {
            try
            {
                nlohmann::json gltfData;
                in >> gltfData;

                if (gltfData.contains("animations"))
                {
                    hasAnimations = true;
                    std::string animationName = gltfData["animations"][0].value("name", "default_morph");
                    std::cout << "[Model] Morph animation detected: " << animationName << std::endl;

                    loadAnimationsFromGLTF(path);
                }
            }
            catch (const std::exception &e)
            {
                std::cerr << "[Model - ERROR] Failed to parse animation JSON: " << e.what() << std::endl;
            }
        }
    }

    processNode(scene->mRootNode, scene);

    std::cout << "[Model] Loaded " << animations.size() << " animations and "
              << animators.size() << " animators.\n";

    return true;
}

void Model::loadAnimationsFromGLTF(const std::string &gltfPath)
{
    std::ifstream file(gltfPath);
    if (!file.is_open())
    {
        std::cerr << "[GLTF] Failed to open: " << gltfPath << std::endl;
        return;
    }

    nlohmann::json gltf;
    try
    {
        file >> gltf;
    }
    catch (const std::exception &e)
    {
        std::cerr << "[GLTF] JSON parsing error: " << e.what() << std::endl;
        return;
    }

    // Load .bin file
    std::string binPath = gltfPath.substr(0, gltfPath.find_last_of('.')) + ".bin";
    std::ifstream bin(binPath, std::ios::binary);
    if (!bin.is_open())
    {
        std::cerr << "[GLTF] Failed to open .bin file: " << binPath << std::endl;
        return;
    }

    std::vector<uint8_t> binData((std::istreambuf_iterator<char>(bin)), {});

    // Validate required sections
    if (!gltf.contains("meshes") || !gltf["meshes"].is_array() || gltf["meshes"].empty())
        return;

    const auto &mesh = gltf["meshes"][0];
    if (!mesh.contains("primitives") || !mesh["primitives"].is_array() || mesh["primitives"].empty())
        return;

    const auto &primitive = mesh["primitives"][0];
    if (!primitive.contains("targets") || !primitive["targets"].is_array())
        return;

    const auto &targets = primitive["targets"];

    // Morph target deltas
    std::vector<std::vector<glm::vec3>> morphDeltas;
    for (const auto &target : targets)
    {
        if (!target.contains("POSITION"))
            continue;

        int accessorIndex = target["POSITION"];
        if (!gltf.contains("accessors") || accessorIndex >= gltf["accessors"].size())
            continue;

        const auto &accessor = gltf["accessors"][accessorIndex];
        int bufferViewIndex = accessor.value("bufferView", -1);
        if (bufferViewIndex == -1 || bufferViewIndex >= gltf["bufferViews"].size())
            continue;

        const auto &bufferView = gltf["bufferViews"][bufferViewIndex];
        size_t count = accessor["count"];
        size_t offset = accessor.value("byteOffset", 0) + bufferView.value("byteOffset", 0);

        std::vector<glm::vec3> deltas;
        for (size_t i = 0; i < count; ++i)
        {
            size_t base = offset + i * sizeof(float) * 3;
            if (base + 12 > binData.size())
                break;

            float x = *reinterpret_cast<float *>(&binData[base]);
            float y = *reinterpret_cast<float *>(&binData[base + 4]);
            float z = *reinterpret_cast<float *>(&binData[base + 8]);
            deltas.emplace_back(x, y, z);
        }
        morphDeltas.push_back(deltas);
    }

    animations.clear();
    animators.clear();

    if (!gltf.contains("animations"))
        return;
    for (const auto &anim : gltf["animations"])
    {
        Animation animation(anim.value("name", "default"));
        animation.morphDeltas = morphDeltas;

        if (!anim.contains("channels") || !anim.contains("samplers"))
            continue;

        for (const auto &channel : anim["channels"])
        {
            if (channel["target"]["path"] != "weights")
                continue;

            int samplerIndex = channel["sampler"];
            if (samplerIndex >= anim["samplers"].size())
                continue;

            const auto &sampler = anim["samplers"][samplerIndex];
            int inputIndex = sampler["input"];
            int outputIndex = sampler["output"];

            if (inputIndex >= gltf["accessors"].size() || outputIndex >= gltf["accessors"].size())
                continue;

            const auto &inputAcc = gltf["accessors"][inputIndex];
            const auto &outputAcc = gltf["accessors"][outputIndex];

            int inputViewIndex = inputAcc.value("bufferView", -1);
            int outputViewIndex = outputAcc.value("bufferView", -1);
            if (inputViewIndex == -1 || outputViewIndex == -1)
                continue;

            const auto &inputView = gltf["bufferViews"][inputViewIndex];
            const auto &outputView = gltf["bufferViews"][outputViewIndex];

            size_t inputCount = inputAcc["count"];
            size_t outputCount = outputAcc["count"];
            size_t inputOffset = inputAcc.value("byteOffset", 0) + inputView.value("byteOffset", 0);
            size_t outputOffset = outputAcc.value("byteOffset", 0) + outputView.value("byteOffset", 0);

            for (size_t i = 0; i < inputCount; ++i)
            {
                size_t timeOffset = inputOffset + i * 4;
                if (timeOffset + 4 > binData.size())
                    break;

                float time = *reinterpret_cast<float *>(&binData[timeOffset]);

                MorphFrame frame;
                frame.time = time;
                frame.weights.resize(morphDeltas.size());

                for (size_t j = 0; j < morphDeltas.size(); ++j)
                {
                    size_t base = outputOffset + (i * morphDeltas.size() + j) * 4;
                    if (base + 4 > binData.size())
                        break;

                    frame.weights[j] = *reinterpret_cast<float *>(&binData[base]);
                }

                animation.keyFrames.push_back(frame);
            }
        }

        const auto &channel = anim["channels"][0];
        int targetNodeIndex = channel["target"]["node"];
        std::string targetMeshName = gltf["nodes"][targetNodeIndex].value("name", ""); // ← this maps node → mesh name

        animation.targetMeshName = targetMeshName;

        std::cout << "[Model - Load] Animation with name " << animation.name << " is associated with mesh with name" << animation.targetMeshName << std::endl;

        animations.push_back(animation);

        Animator animator;
        animator.Play(&animations.back());
        animators.push_back(animator);

        if (animations.back().keyFrames.empty())
        {
            std::cerr << "[ANIM ERROR] No keyframes in animation\n";
        }
        for (const auto &frame : animations.back().keyFrames)
        {
            std::cerr << "[DEBUG] Frame time: " << frame.time
                      << ", weights size: " << frame.weights.size() << "\n";
        }
    }
}
