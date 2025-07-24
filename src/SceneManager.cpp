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
    return NodeType::Empty;
}

SceneManager::SceneManager(const std::string &projectPath) : projectPath(projectPath)
{
    nodes.push_back(root);
    nextID = 1;

    std::cout << "[SceneManager] Initializing SceneManager with project path: " << projectPath << std::endl;
}

void SceneManager::addToParent(std::string &name, NodeType type, unsigned int parentID, unsigned int assignedID)
{
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

void SceneManager::addToParent(std::string &name, std::string &filepath, NodeType type, unsigned int parentID, unsigned int assignedID)
{
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

void SceneManager::RenderModels(Shader &shader)
{
    for (auto &model : models)
    {
        glm::mat4 modelMat = model.getModelMatrix();
        shader.setUniforms("model", static_cast<unsigned int>(UniformType::Mat4f), glm::value_ptr(modelMat));
        model.Draw(shader);
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

    std::cout << "[SceneManager] Deleting node with ID: " << nodeToDelete->ID << " and name: " << nodeToDelete->name << std::endl;
    nodes.erase(std::remove(nodes.begin(), nodes.end(), nodeToDelete), nodes.end());
    delete nodeToDelete;
}

Model *SceneManager::getModelByID(unsigned int ID)
{
    for (Model &model : models)
    {
        if (model.ID == ID)
        {
            return &model;
        }
    }
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
            }
            else
            {
                std::cerr << "[SceneManager] Warning: No model found for node ID " << node->ID << std::endl;
            }
        }

        // Save light-specific data
        else if (node->type == NodeType::Light)
        {
            j["intensity"] = 1.0;
            j["direction"] = {0.0, -1.0, 0.0};
            j["color"] = {1.0, 1.0, 1.0};
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
    nextID = 1;

    std::function<void(json &, Node *)> buildNodeRecursive = [&](json &j, Node *parent)
    {
        unsigned int id = j["id"];
        std::string name = j["name"];
        NodeType type = stringToNodeType(j["type"]);

        if (type == NodeType::Model && j.contains("modelPath"))
        {
            std::string modelPath = j["modelPath"];
            addToParent(name, modelPath, type, parent->ID, id);
        }
        else
        {
            addToParent(name, type, parent->ID, id);
        }

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