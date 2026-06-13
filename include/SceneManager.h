#pragma once

#include <unordered_map>
#include <vector>
#include <iostream>
#include <string>
#include <dirent.h>
#include <mutex>
#include <atomic>

#include <json.hpp>
#include <glm/glm.hpp>

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
    Empty,
    Camera
};

struct Node
{
    unsigned int ID;
    std::string name;

    NodeType type;

    Node *parent = nullptr;
    std::vector<Node *> children;

    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);
};

class SceneManager
{
public:
    unsigned int nextID;

    Node *root = nullptr;
    std::vector<Node *> nodes;
    std::unordered_map<unsigned int, Node *> nodeMap;
    std::unordered_map<unsigned int, Model> models;
    std::unordered_map<unsigned int, Light> lights;
    std::unordered_map<unsigned int, ParticleEmitter> particleEmitters;
    std::unordered_map<unsigned int, btRigidBody *> rigidBodies;

    ShaderManager *sm = nullptr;
    PhysicsEngine *physics = nullptr;
    std::atomic<int> physicsCounter{0};
    Mesh *cameraMesh = nullptr;

    bool drawLights = true,
         drawPhysics = true,
         infiniteFloor = true,
         simulate = false;

    unsigned int activeCameraID = 0; // 0 = Viewport Camera, otherwise Node ID of NodeType::Camera
    bool cameraChangesPending = false;
    glm::vec3 pendingCamPos = glm::vec3(0.0f);
    float pendingCamYaw = -90.0f;
    float pendingCamPitch = 0.0f;

    glm::vec3 viewportCamPos = glm::vec3(0.0f, 0.0f, 3.0f);
    float viewportYaw = -90.0f;
    float viewportPitch = 0.0f;

    bool isDirty = false;
    std::vector<nlohmann::json> undoStack;
    std::vector<nlohmann::json> redoStack;

    nlohmann::json serializeScene();
    void deserializeScene(const nlohmann::json &data);
    void pushUndoState();
    void undo();
    void redo();

    std::string nodeTypeToString(NodeType type);
    NodeType stringToNodeType(const std::string &str);
    std::string lightTypeToString(LightType type);
    LightType stringToLightType(const std::string &str);

    SceneManager(const std::string &projectPath);
    ~SceneManager();

    // add a light node to parent
    void addToParent(std::string &name, NodeType type, unsigned int parentID, LightType lightType, unsigned int forcedID = 0);

    // add a model node to parent
    void addToParent(std::string &name, std::string &filepath, NodeType type, unsigned int parentID, unsigned int forcedID = 0);

    // add a particle system
    void addToParent(std::string &name, NodeType type, unsigned int parentID, std::string &shaderName, unsigned int maxParticles, unsigned int forcedID = 0);

    // add a rigid body
    void addToParent(std::string &name, NodeType type, unsigned int parentID, RigidBodyShape shape, float mass, unsigned int forcedID = 0);

    // add any other node to parent
    void addToParent(std::string &name, NodeType type, unsigned int parentID, unsigned int forcedID = 0);

    void RenderModels(Shader &shader, float deltaTime);
    void RenderLights(Shader &shader);
    void RenderParticles(float dt);
    void RenderPhysics(float dt, Shader &shader);
    void RenderCameras(Shader &shader);

    void deleteNode(unsigned int ID);

    Model *getModelByID(unsigned int ID);
    Light *getLightByID(unsigned int ID);
    ParticleEmitter *getEmitterByID(unsigned int ID);
    btRigidBody *getRigidBodyByID(unsigned int ID);
    Node *getCameraNode();

    struct PendingModelLoad
    {
        std::string name;
        std::string filepath;
        NodeType type;
        unsigned int assignedID;
        Model *modelPtr;
    };

    void addToParentAsync(std::string &name, std::string &filepath, NodeType type, unsigned int parentID);
    void UpdateAsyncLoads();

    void saveScene();
    void LoadScene(const std::string &path);
    void ResetPhysics();
    void SyncTransforms();

    unsigned int findNextAvailableID();
    Node *find_node(unsigned int ID);
    glm::mat4 getWorldTransform(unsigned int ID);

private:
    const std::string projectPath;
    std::string projectName;

    std::vector<PendingModelLoad> pendingModelLoads;
    std::mutex pendingLoadsMutex;
    std::unordered_map<unsigned int, btTransform> initialTransforms;

    std::unordered_map<unsigned int, glm::vec3> initialNodePositions;
    std::unordered_map<unsigned int, glm::vec3> initialNodeRotations;
    std::unordered_map<unsigned int, glm::vec3> initialNodeScales;
    bool m_wasSimulating = false;

    void initializeChildTransform(Node *newNode);
};