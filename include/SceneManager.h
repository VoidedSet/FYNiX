#pragma once

#include <vector>
#include <iostream>
#include <string>
#include <dirent.h>

#include <json.hpp>

#include "Model.h"
#include "Light.h"

enum class NodeType
{
    Root,
    Model,
    Light,
    Empty
};

struct Node
{
    unsigned int ID;
    std::string name;

    NodeType type;

    Node *parent = nullptr;
    std::vector<Node *> children;
};

class SceneManager
{
public:
    unsigned int nextID;

    Node *root = new Node({0,
                           "Root",
                           NodeType::Root,
                           nullptr,
                           {}});
    std::vector<Node *> nodes;
    std::vector<Model> models;
    std::vector<Light> lights;

    bool drawLights = true;

    std::string nodeTypeToString(NodeType type);
    NodeType stringToNodeType(const std::string &str);

    SceneManager(const std::string &projectPath);

    // add a light node to parent
    void addToParent(std::string &name, NodeType type, unsigned int parentID, LightType lightType);

    // add a model node to parent
    void addToParent(std::string &name, std::string &filepath, NodeType type, unsigned int assignedID);

    // add any other node to parent
    void addToParent(std::string &name, NodeType type, unsigned int parentID);

    void RenderModels(Shader &shader);
    void RenderLights(Shader &shader);

    void deleteNode(unsigned int ID);

    Model *getModelByID(unsigned int ID);
    Light *getLightById(unsigned int ID);

    void saveScene();
    void LoadScene(const std::string &path);

    Node *find_node(unsigned int ID);

private:
    const std::string projectPath;
    std::string projectName;
};