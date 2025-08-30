#pragma once

#include <unordered_map>
#include <vector>
#include <iostream>
#include <string>
#include <dirent.h>

#include <json.hpp>

#include "Model.h"
#include "Light.h"
#include "ParticleSystem.h"
#include "ShaderManager.h"

#include "PhysicsEngine.h"

enum class NodeType
{
    Root,
    Model,
    Light,
    Particles,
    RigidBody,
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
    std::vector<ParticleEmitter> particleEmitters;
    std::unordered_map<unsigned int, btRigidBody *> rigidBodies;

    ShaderManager *sm = nullptr;
    PhysicsEngine *physics = nullptr;

    bool drawLights = true,
         drawPhysics = true,
         simulate = true;

    std::string nodeTypeToString(NodeType type);
    NodeType stringToNodeType(const std::string &str);

    SceneManager(const std::string &projectPath);

    // add a light node to parent
    void addToParent(std::string &name, NodeType type, unsigned int parentID, LightType lightType);

    // add a model node to parent
    void addToParent(std::string &name, std::string &filepath, NodeType type, unsigned int assignedID);

    // add a particle system
    void addToParent(std::string &name, NodeType type, unsigned int parentID, std::string &shaderName, unsigned int maxParticles);

    // add a rigid body
    void addToParent(std::string &name, NodeType type, unsigned int parentID, RigidBodyShape shape, float mass);

    // add any other node to parent
    void addToParent(std::string &name, NodeType type, unsigned int parentID);

    void RenderModels(Shader &shader, float deltaTime);
    void RenderLights(Shader &shader);
    void RenderParticles(float dt);
    void RenderPhysics(float dt, Shader &shader);

    void deleteNode(unsigned int ID);

    Model *getModelByID(unsigned int ID);
    Light *getLightByID(unsigned int ID);
    ParticleEmitter *getEmitterByID(unsigned int ID);
    btRigidBody *getRigidBodyByID(unsigned int ID);

    void saveScene();
    void LoadScene(const std::string &path);

    Node *find_node(unsigned int ID);

private:
    const std::string projectPath;
    std::string projectName;
};