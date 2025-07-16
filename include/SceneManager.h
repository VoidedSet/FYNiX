#pragma once

#include <vector>
#include <iostream>
#include <string>

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

    SceneManager();
    void addToParent(std::string &name, NodeType type, unsigned int parentID);
    void addToParent(std::string &name, std::string &filepath, NodeType type, unsigned int parentID);
    void RenderModels(Shader &shader);

private:
    unsigned int nextID;

    Node *find_node(unsigned int ID);
};