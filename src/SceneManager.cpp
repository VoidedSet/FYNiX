#include "SceneManager.h"

SceneManager::SceneManager()
{
    nodes.push_back(root);
    nextID = 1;

    std::cout << "[SceneManager] SceneManager initialized Scene with root node." << std::endl;
}

void SceneManager::addToParent(std::string &name, NodeType type, unsigned int parentID)
{
    Node *parentNode = find_node(parentID);
    if (!parentNode)
    {
        std::cerr << "[SceneManager] Error: Parent node with ID " << parentID << " not found." << std::endl;
        return;
    }

    Node *newNode = new Node({nextID++, name + " ID: " + std::to_string(nextID - 1), type, parentNode, {}});
    nodes.push_back(newNode);
    parentNode->children.push_back(newNode);

    std::cout << "[SceneManager] Added new node with ID: " << newNode->ID << " and name: " << newNode->name << std::endl;
}

void SceneManager::addToParent(std::string &name, std::string &filepath, NodeType type, unsigned int parentID)
{
    Node *parentNode = find_node(parentID);
    if (!parentNode)
    {
        std::cerr << "[SceneManager] Error: Parent node with ID " << parentID << " not found." << std::endl;
        return;
    }

    Node *newNode = new Node({nextID++, name + " ID: " + std::to_string(nextID - 1), type, parentNode, {}});
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
    for (Model &model : models)
    {
        model.Draw(shader);
    }
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
