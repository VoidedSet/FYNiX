#pragma once

#include <vector>
#include <iostream>
#include <string>
#include <dirent.h>

#include <json.hpp>

#include "Model.h"

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
    Node *root = new Node({0,
                           "Root",
                           NodeType::Root,
                           nullptr,
                           {}});
    std::vector<Node *> nodes;
    std::vector<Model> models;

    SceneManager(const std::string &projectPath);
    void addToParent(std::string &name, NodeType type, unsigned int parentID);
    void addToParent(std::string &name, std::string &filepath, NodeType type, unsigned int parentID);
    void RenderModels(Shader &shader);

    void saveScene();
    void LoadScene(const std::string &path);

private:
    unsigned int nextID;
    const std::string projectPath;
    std::string projectName;

    Node *find_node(unsigned int ID);
};