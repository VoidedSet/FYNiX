#include "SceneManager.h"

using json = nlohmann::json;

std::string SceneManager::nodeTypeToString(NodeType type)
{
    switch (type)
    {
    case NodeType::Empty:
        return "Empty";
    case NodeType::Model:
        return "Model";
    case NodeType::Light:
        return "Light";
    case NodeType::Root:
        return "Root";
    case NodeType::Particles:
        return "ParticleEmitter";
    default:
        return "Unknown";
    }
}

NodeType SceneManager::stringToNodeType(const std::string &str)
{
    if (str == "Empty")
        return NodeType::Empty;
    if (str == "Model")
        return NodeType::Model;
    if (str == "Light")
        return NodeType::Light;
    if (str == "Root")
        return NodeType::Root;
    if (str == "ParticleEmitter")
        return NodeType::Particles;
    return NodeType::Empty;
}

SceneManager::SceneManager(const std::string &projectPath) : projectPath(projectPath)
{
    nodes.push_back(root);
    nextID = 1;

    std::cout << "[SceneManager] Initializing SceneManager with project path: " << projectPath << std::endl;
}

void SceneManager::addToParent(std::string &name, NodeType type, unsigned int parentID, LightType lightType)
{
    unsigned int assignedID = nextID;
    Node *parentNode = find_node(parentID);
    if (!parentNode)
    {
        std::cerr << "[SceneManager] Error: Parent node with ID " << parentID << " not found." << std::endl;
        return;
    }

    nextID = assignedID + 1;
    Node *newNode = new Node({assignedID, name, type, parentNode, {}});
    nodes.push_back(newNode);
    parentNode->children.push_back(newNode);

    if (type == NodeType::Light)
    {
        Light light(newNode->ID, lightType);
        lights.push_back(light);
    }
    std::cout << "[SceneManager] Added new node with ID: " << newNode->ID << " and name: " << newNode->name << std::endl;
}

void SceneManager::addToParent(std::string &name, std::string &filepath, NodeType type, unsigned int parentID)
{
    unsigned int assignedID = nextID;

    Node *parentNode = find_node(parentID);
    if (!parentNode)
    {
        std::cerr << "[SceneManager] Error: Parent node with ID " << parentID << " not found." << std::endl;
        return;
    }
    nextID = assignedID + 1;
    Node *newNode = new Node({assignedID, name, type, parentNode, {}});
    nodes.push_back(newNode);
    parentNode->children.push_back(newNode);

    if (type == NodeType::Model)
    {
        Model model(filepath, newNode->ID);
        models.push_back(model);
        std::cout << "[SceneManager] Model loaded and added to node with ID: " << newNode->ID << std::endl;
    }

    std::cout << "[SceneManager] Added new node with ID: " << newNode->ID << " and name: " << newNode->name << std::endl;
}

void SceneManager::addToParent(std::string &name, NodeType type, unsigned int parentID, std::string &shaderName, unsigned int maxParticles)
{
    unsigned int assignedID = nextID;
    Node *parentNode = find_node(parentID);
    if (!parentNode)
    {
        std::cerr << "[SceneManager] Error: Parent node with ID " << parentID << " not found." << std::endl;
        return;
    }

    nextID = assignedID + 1;
    Node *newNode = new Node({assignedID, name, type, parentNode, {}});
    nodes.push_back(newNode);
    parentNode->children.push_back(newNode);

    if (type == NodeType::Particles)
    {
        ParticleEmitter particleEmitter(sm->findShader("particle"), maxParticles, assignedID);
        particleEmitters.push_back(particleEmitter);

        addToParent(name, NodeType::Light, assignedID, LightType::POINTLIGHT);
    }

    std::cout << "[SceneManager] Added new node with ID: " << newNode->ID << " and name: " << newNode->name << std::endl;
}

void SceneManager::addToParent(std::string &name, NodeType type, unsigned int parentID)
{
    unsigned int assignedID = nextID;
    Node *parentNode = find_node(parentID);
    if (!parentNode)
    {
        std::cerr << "[SceneManager] Error: Parent node with ID " << parentID << " not found." << std::endl;
        return;
    }

    nextID = assignedID + 1;
    Node *newNode = new Node({assignedID, name, type, parentNode, {}});
    nodes.push_back(newNode);
    parentNode->children.push_back(newNode);

    std::cout << "[SceneManager] Added new node with ID: " << newNode->ID << " and name: " << newNode->name << std::endl;
}

void SceneManager::RenderModels(Shader &shader, float deltaTime)
{
    int lightCount = lights.size();
    shader.setUniforms("numLights", (unsigned int)UniformType::Int, &lightCount);

    for (int i = 0; i < lightCount; ++i)
    {
        std::string posName = "lightPositions[" + std::to_string(i) + "]";
        std::string colName = "lightColors[" + std::to_string(i) + "]";

        shader.setUniforms(posName.c_str(), (unsigned int)UniformType::Vec3f, (void *)(glm::value_ptr(lights[i].position)));
        shader.setUniforms(colName.c_str(), (unsigned int)UniformType::Vec3f, (void *)(glm::value_ptr(lights[i].color)));
    }

    for (Model &model : models)
    {
        if (model.hasAnimation)
            model.UpdateAnimation(deltaTime);

        glm::mat4 modelMat = model.getModelMatrix();
        shader.setUniforms("model", (unsigned int)UniformType::Mat4f, glm::value_ptr(modelMat));
        model.Draw(shader);
    }
}

void SceneManager::RenderLights(Shader &shader)
{
    if (drawLights)
        for (auto &light : lights)
        {
            shader.use();
            light.Draw(shader);
        }
}

void SceneManager::RenderParticles(float dt)
{
    for (auto &emitter : particleEmitters)
    {
        for (int i = 0; i < 10; i++)
        {
            Particle newParticle;
            newParticle.Position = glm::vec3(0.0f, 0.0f, 0.0f);
            newParticle.Velocity = glm::vec3((rand() % 100 - 50) / 10.0f, 5.f, (rand() % 100 - 50) / 10.0f);
            newParticle.Life = 1.5f;
            // newParticle.Color = glm::vec4(1.0f, 0.5f, 0.2f, 1.0f);
            newParticle.Color = emitter.Color;
            newParticle.Size = 0.05f;
            emitter.SpawnParticle(newParticle);
        }

        emitter.Update(dt);
        emitter.Draw();
    }
}

void SceneManager::deleteNode(unsigned int ID)
{
    if (ID == root->ID)
    {
        std::cerr << "[SceneManager] Cannot delete root node." << std::endl;
        return;
    }
    Node *nodeToDelete = find_node(ID);
    if (!nodeToDelete)
    {
        std::cerr << "[SceneManager] Error: Node with ID " << ID << " not found." << std::endl;
        return;
    }

    if (nodeToDelete->parent)
    {
        auto &siblings = nodeToDelete->parent->children;
        siblings.erase(std::remove(siblings.begin(), siblings.end(), nodeToDelete), siblings.end());
    }

    if (!nodeToDelete->children.empty())
    {
        for (auto &childern : nodeToDelete->children)
        {
            childern->parent = nodeToDelete->parent;
            nodeToDelete->parent->children.push_back(childern);
        }

        nodeToDelete->children.clear();
    }

    if (nodeToDelete->type == NodeType::Model)
    {
        auto it = std::find_if(models.begin(), models.end(), [&](const Model &m)
                               { return m.ID == nodeToDelete->ID; });
        if (it != models.end())
        {
            std::cout << "[SceneManager] Deleting model with ID: " << it->ID << " and path: " << it->directory << std::endl;
            models.erase(it);
        }
        else
        {
            std::cerr << "[SceneManager] Warning: No model found for node ID " << nodeToDelete->ID << std::endl;
        }
    }

    if (nodeToDelete->type == NodeType::Light)
    {
        auto it = std::find_if(lights.begin(), lights.end(), [&](const Light &m)
                               { return m.ID == nodeToDelete->ID; });
        if (it != lights.end())
        {
            std::cout << "[SceneManager] Deleting Light with ID: " << it->ID << std::endl;
            lights.erase(it);
        }
        else
            std::cerr << "[SceneManager] Warning: No Light found for node ID " << nodeToDelete->ID << std::endl;
    }
    if (nodeToDelete->type == NodeType::Particles)
    {
        if (!nodeToDelete->children.empty())
        {
            deleteNode(nodeToDelete->children[0]->ID);
        }
        auto it = std::find_if(particleEmitters.begin(), particleEmitters.end(), [&](const ParticleEmitter &m)
                               { return m.ID == nodeToDelete->ID; });
        if (it != particleEmitters.end())
        {
            std::cout << "[SceneManager] Deleting Particle Emitter with ID: " << it->ID << std::endl;
            // deleteNode(nodeToDelete->children[0]->ID);
            particleEmitters.erase(it);
        }
        else
            std::cerr << "[SceneManager] Warning: No Particle Emitter found for node ID " << nodeToDelete->ID << std::endl;
    }

    std::cout << "[SceneManager] Deleting node with ID: " << nodeToDelete->ID << " and name: " << nodeToDelete->name << std::endl;
    nodes.erase(std::remove(nodes.begin(), nodes.end(), nodeToDelete), nodes.end());
    delete nodeToDelete;
}

Model *SceneManager::getModelByID(unsigned int ID)
{
    for (Model &model : models)
        if (model.ID == ID)
            return &model;
    return nullptr;
}

Light *SceneManager::getLightByID(unsigned int ID)
{
    for (Light &light : lights)
        if (light.ID == ID)
            return &light;
    return nullptr;
}

ParticleEmitter *SceneManager::getEmitterByID(unsigned int ID)
{
    for (ParticleEmitter &emitter : particleEmitters)
        if (emitter.ID == ID)
            return &emitter;
    return nullptr;
}

void SceneManager::saveScene()
{
    if (!root)
    {
        std::cerr << "[SceneManager] Cannot save scene: root is null." << std::endl;
        return;
    }

    json outJson;
    outJson["projectName"] = projectName;

    std::function<json(Node *)> buildNodeJson = [&](Node *node) -> json
    {
        json j;
        j["id"] = node->ID;
        j["name"] = node->name;
        j["type"] = nodeTypeToString(node->type);

        // Save model-specific data
        if (node->type == NodeType::Model)
        {
            auto it = std::find_if(models.begin(), models.end(), [&](const Model &m)
                                   { return m.ID == node->ID; });

            if (it != models.end())
            {
                if (!it->directory.empty())
                {
                    j["modelPath"] = it->directory;
                }
                else
                {
                    std::cerr << "[SceneManager] Warning: Model with ID " << node->ID << " has empty directory." << std::endl;
                }
                j["position"] = {it->getPosition().x, it->getPosition().y, it->getPosition().z};
                j["rotation"] = {it->getRotation().x, it->getRotation().y, it->getRotation().z};
                j["scale"] = {it->getScale().x, it->getScale().y, it->getScale().z};
            }
            else
            {
                std::cerr << "[SceneManager] Warning: No model found for node ID " << node->ID << std::endl;
            }
        }

        // Save light-specific data
        else if (node->type == NodeType::Light)
        {
            auto it = std::find_if(lights.begin(), lights.end(), [&](const Light &l)
                                   { return l.ID == node->ID; });

            if (it != lights.end())
            {
                j["color"] = {it->color.x, it->color.y, it->color.z};
                j["position"] = {it->position.x, it->position.y, it->position.z};
            }
            else
            {
                std::cerr << "[SceneManager] Warning: No light found for node ID " << node->ID << std::endl;
            }
        }

        else if (node->type == NodeType::Particles)
        {
            auto it = std::find_if(particleEmitters.begin(), particleEmitters.end(), [&](const ParticleEmitter &p)
                                   { return p.ID == node->ID; });

            if (it != particleEmitters.end())
            {
                j["color"] = {it->Color.r, it->Color.g, it->Color.b, it->Color.a};
                j["position"] = {it->Position.x, it->Position.y, it->Position.z};
                j["shdaerName"] = it->shader.Name;
                j["maxParticles"] = it->maxParticles;
            }
            else
            {
                std::cerr << "[SceneManager] Warning: No Particle Emitter found for node ID " << node->ID << std::endl;
            }
        }

        // Recurse into children
        if (!node->children.empty())
        {
            j["children"] = json::array();
            for (Node *child : node->children)
            {
                j["children"].push_back(buildNodeJson(child));
            }
        }

        return j;
    };

    outJson["scenegraph"] = buildNodeJson(root);

    std::ofstream outFile(projectPath);
    if (!outFile.is_open())
    {
        std::cerr << "[SceneManager] Failed to open file for writing: " << projectPath << std::endl;
        return;
    }

    outFile << std::setw(2) << outJson << std::endl;

    if (!outFile.good())
    {
        std::cerr << "[SceneManager] Error occurred while writing to file: " << projectPath << std::endl;
    }
    else
    {
        std::cout << "[SceneManager] Scene saved successfully to: " << projectPath << std::endl;
    }
}

void SceneManager::LoadScene(const std::string &path)
{
    std::cout << "[SceneManager] Loading scene from path: " << path << std::endl;

    std::ifstream file(path);
    if (!file.is_open())
    {
        std::cerr << "[SceneManager] Failed to open file: " << path << std::endl;
        return;
    }

    json data;
    try
    {
        file >> data;
    }
    catch (const std::exception &e)
    {
        std::cerr << "[SceneManager] Failed to parse JSON: " << e.what() << std::endl;
        return;
    }

    if (!data.contains("scenegraph"))
    {
        std::cerr << "[SceneManager] Malformed .fynx file: missing 'scenegraph'" << std::endl;
        return;
    }

    projectName = data.value("projectName", "UnnamedProject");

    // Cleanup
    for (Node *node : nodes)
    {
        delete node;
    }
    nodes.clear();
    models.clear();
    lights.clear(); // Add this if lights are persistent
    nextID = 1;

    std::function<void(json &, Node *)> buildNodeRecursive = [&](json &j, Node *parent)
    {
        unsigned int id = j["id"];
        std::string name = j["name"];
        NodeType type = stringToNodeType(j["type"]);

        if (type == NodeType::Model && j.contains("modelPath"))
        {
            std::string modelPath = j["modelPath"];
            addToParent(name, modelPath, type, parent->ID);

            // Set model transform if available
            auto *model = getModelByID(id);
            if (model)
            {
                if (j.contains("position"))
                    model->setPosition(glm::vec3(j["position"][0], j["position"][1], j["position"][2]));
                if (j.contains("rotation"))
                    model->setRotation(glm::vec3(j["rotation"][0], j["rotation"][1], j["rotation"][2]));
                if (j.contains("scale"))
                    model->setScale(glm::vec3(j["scale"][0], j["scale"][1], j["scale"][2]));
            }
        }
        else if (type == NodeType::Light)
        {
            if (parent->type == NodeType::Particles)
                return;
            addToParent(name, type, parent->ID, LightType::DIRECTIONAL);

            // Set light data if available
            auto *light = getLightByID(id);
            if (light)
            {
                if (j.contains("position"))
                    light->position = glm::vec3(j["position"][0], j["position"][1], j["position"][2]);
                if (j.contains("color"))
                    light->color = glm::vec3(j["color"][0], j["color"][1], j["color"][2]);
            }
        }
        else if (type == NodeType::Particles)
        {
            std::string shaderName;
            unsigned int maxParticles = 0;

            if (j.contains("shaderName"))
                shaderName = j["shaderName"];
            if (j.contains("maxParticles"))
                maxParticles = j["maxParticles"];

            addToParent(name, type, parent->ID, shaderName, maxParticles);

            auto *emitter = getEmitterByID(id);
            if (emitter)
            {
                if (j.contains("position"))
                    emitter->Position = glm::vec3(j["position"][0], j["position"][1], j["position"][2]);
                if (j.contains("color"))
                    emitter->Color = glm::vec4(j["color"][0], j["color"][1], j["color"][2], j["color"][3]);
            }
        }
        else
        {
            addToParent(name, type, parent->ID);
        }

        // Recurse into children
        if (j.contains("children"))
        {
            for (auto &childJson : j["children"])
            {
                buildNodeRecursive(childJson, find_node(id));
            }
        }
    };

    // Rebuild root node manually
    json rootJson = data["scenegraph"];
    unsigned int rootID = rootJson["id"];
    std::string rootName = rootJson["name"];
    NodeType rootType = stringToNodeType(rootJson["type"]);

    root = new Node{rootID, rootName, rootType, nullptr, {}};
    nodes.push_back(root);
    nextID = std::max(nextID, rootID + 1);

    if (rootJson.contains("children"))
    {
        for (auto &childJson : rootJson["children"])
        {
            buildNodeRecursive(childJson, root);
        }
    }

    std::cout << "[SceneManager] Scene loaded successfully." << std::endl;
}

Node *SceneManager::find_node(unsigned int id)
{
    if (id == root->ID)
        return root;

    if (id < 0 || id >= nextID)
    {
        std::cerr << "[SceneManager] Error: Invalid ID " << id << " requested." << std::endl;
        return nullptr;
    }

    for (Node *node : nodes)
    {
        if (node->ID == id)
            return node;
    }

    return nullptr;
}