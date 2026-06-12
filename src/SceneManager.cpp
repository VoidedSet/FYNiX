#include "SceneManager.h"
#include "JobSystem.h"
#include <chrono>
#include <vector>
#include <glm/gtc/type_ptr.hpp>
#include "Camera.h"

using namespace std;

namespace
{
    const std::vector<std::string> lightPosUniformNames = []() {
        std::vector<std::string> names;
        for (int i = 0; i < 64; ++i)
        {
            names.push_back("lightPositions[" + std::to_string(i) + "]");
        }
        return names;
    }();

    const std::vector<std::string> lightColUniformNames = []() {
        std::vector<std::string> names;
        for (int i = 0; i < 64; ++i)
        {
            names.push_back("lightColors[" + std::to_string(i) + "]");
        }
        return names;
    }();

    glm::vec3 extractXYZ(const glm::mat3& R) {
        float ey = asin(glm::clamp(R[2][0], -1.0f, 1.0f));
        float ex, ez;
        if (cos(ey) > 0.0001f) {
            ex = atan2(-R[2][1], R[2][2]);
            ez = atan2(-R[1][0], R[0][0]);
        } else {
            ex = atan2(R[1][2], R[1][1]);
            ez = 0.0f;
        }
        return glm::vec3(ex, ey, ez);
    }
}

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
    case NodeType::RigidBody:
        return "RigidBody";
    case NodeType::Camera:
        return "Camera";
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
    if (str == "RigidBody")
        return NodeType::RigidBody;
    if (str == "Camera")
        return NodeType::Camera;
    return NodeType::Empty;
}

SceneManager::SceneManager(const std::string &projectPath) : projectPath(projectPath)
{
    root = new Node{0, "Root", NodeType::Root, nullptr, {}};
    nodes.push_back(root);
    nodeMap[root->ID] = root;
    nextID = 1;

    std::cout << "[SceneManager] Initializing SceneManager with project path: " << projectPath << std::endl;

    // physics = new PhysicsEngine;
}

SceneManager::~SceneManager()
{
    std::cout << "[SceneManager] Shutting down and deleting all scene nodes." << std::endl;
    for (Node *node : nodes)
    {
        delete node;
    }
    nodes.clear();
    nodeMap.clear();

    if (physics)
    {
        delete physics;
        physics = nullptr;
    }
}

void SceneManager::addToParent(std::string &name, NodeType type, unsigned int parentID, LightType lightType, unsigned int forcedID)
{
    unsigned int assignedID = (forcedID != 0) ? forcedID : findNextAvailableID();
    Node *parentNode = find_node(parentID);
    if (!parentNode)
    {
        std::cerr << "[SceneManager] Error: Parent node with ID " << parentID << " not found." << std::endl;
        return;
    }

    if (assignedID >= nextID)
        nextID = assignedID + 1;
    Node *newNode = new Node({assignedID, name, type, parentNode, {}});
    nodes.push_back(newNode);
    nodeMap[newNode->ID] = newNode;
    parentNode->children.push_back(newNode);

    if (type == NodeType::Light)
    {
        Light light(newNode->ID, lightType);
        lights.emplace(newNode->ID, std::move(light));
    }
    if (forcedID == 0)
        initializeChildTransform(newNode);
    std::cout << "[SceneManager] Added new node with ID: " << newNode->ID << " and name: " << newNode->name << std::endl;
}

void SceneManager::addToParent(std::string &name, std::string &filepath, NodeType type, unsigned int parentID, unsigned int forcedID)
{
    unsigned int assignedID = (forcedID != 0) ? forcedID : findNextAvailableID();

    Node *parentNode = find_node(parentID);
    if (!parentNode)
    {
        std::cerr << "[SceneManager] Error: Parent node with ID " << parentID << " not found." << std::endl;
        return;
    }
    if (assignedID >= nextID)
        nextID = assignedID + 1;
    Node *newNode = new Node({assignedID, name, type, parentNode, {}});
    nodes.push_back(newNode);
    nodeMap[newNode->ID] = newNode;
    parentNode->children.push_back(newNode);

    if (type == NodeType::Model)
    {
        Model model(filepath, newNode->ID);
        models.emplace(newNode->ID, std::move(model));
        std::cout << "[SceneManager] Model loaded and added to node with ID: " << newNode->ID << std::endl;
    }
    if (forcedID == 0)
        initializeChildTransform(newNode);

    std::cout << "[SceneManager] Added new node with ID: " << newNode->ID << " and name: " << newNode->name << std::endl;
}

void SceneManager::addToParentAsync(std::string &name, std::string &filepath, NodeType type, unsigned int parentID)
{
    unsigned int assignedID = findNextAvailableID();

    Node *parentNode = find_node(parentID);
    if (!parentNode)
    {
        std::cerr << "[SceneManager] Error: Parent node with ID " << parentID << " not found." << std::endl;
        return;
    }
    if (assignedID >= nextID)
        nextID = assignedID + 1;
    Node *newNode = new Node({assignedID, name, type, parentNode, {}});
    nodes.push_back(newNode);
    nodeMap[newNode->ID] = newNode;
    parentNode->children.push_back(newNode);

    if (type == NodeType::Model)
    {
        Job job;
        job.completionCounter = nullptr;
        job.work = [this, filepath, assignedID]() {
            Model *modelPtr = new Model(filepath, assignedID, false); // Load CPU data only (no OpenGL)
            
            std::lock_guard<std::mutex> lock(this->pendingLoadsMutex);
            this->pendingModelLoads.push_back({"", filepath, NodeType::Model, assignedID, modelPtr});
        };
        JobSystem::Get().Submit(job);
        std::cout << "[SceneManager] Dispatched async model load for: " << filepath << std::endl;
    }

    initializeChildTransform(newNode);

    std::cout << "[SceneManager] Added new node with ID: " << newNode->ID << " and name: " << newNode->name << " (loading async...)" << std::endl;
}

void SceneManager::UpdateAsyncLoads()
{
    std::vector<PendingModelLoad> readyLoads;
    {
        std::lock_guard<std::mutex> lock(pendingLoadsMutex);
        if (!pendingModelLoads.empty())
        {
            readyLoads = std::move(pendingModelLoads);
            pendingModelLoads.clear();
        }
    }

    for (auto &load : readyLoads)
    {
        load.modelPtr->UploadToGPU(); // Setup VAO, VBO, EBO, and textures on main thread (with OpenGL context)
        Node *n = find_node(load.assignedID);
        if (n)
        {
            load.modelPtr->setPosition(n->position);
            load.modelPtr->setRotation(n->rotation);
            load.modelPtr->setScale(n->scale);
        }
        models.emplace(load.assignedID, std::move(*(load.modelPtr)));
        delete load.modelPtr;
        std::cout << "[SceneManager] Async model loaded and uploaded to GPU: " << load.filepath << std::endl;
    }
}

void SceneManager::addToParent(std::string &name, NodeType type, unsigned int parentID, std::string &shaderName, unsigned int maxParticles, unsigned int forcedID)
{
    unsigned int assignedID = (forcedID != 0) ? forcedID : findNextAvailableID();
    Node *parentNode = find_node(parentID);
    if (!parentNode)
    {
        std::cerr << "[SceneManager] Error: Parent node with ID " << parentID << " not found." << std::endl;
        return;
    }

    if (assignedID >= nextID)
        nextID = assignedID + 1;
    Node *newNode = new Node({assignedID, name, type, parentNode, {}});
    nodes.push_back(newNode);
    nodeMap[newNode->ID] = newNode;
    parentNode->children.push_back(newNode);

    if (type == NodeType::Particles)
    {
        ParticleEmitter particleEmitter(sm->findShader("particle"), maxParticles, assignedID);
        particleEmitters.emplace(assignedID, std::move(particleEmitter));

        addToParent(name, NodeType::Light, assignedID, LightType::POINTLIGHT);
    }
    if (forcedID == 0)
        initializeChildTransform(newNode);

    std::cout << "[SceneManager] Added new node with ID: " << newNode->ID << " and name: " << newNode->name << std::endl;
}

void SceneManager::addToParent(std::string &name, NodeType type, unsigned int parentID, RigidBodyShape shape, float mass, unsigned int forcedID)
{
    unsigned int assignedID = (forcedID != 0) ? forcedID : findNextAvailableID();
    Node *parentNode = find_node(parentID);
    if (!parentNode)
    {
        std::cerr << "[SceneManager] Error: Parent node with ID " << parentID << " not found." << std::endl;
        return;
    }

    if (physics == nullptr)
    {
        physics = new PhysicsEngine;
        if (!physics)
        {
            std::cerr << "[Physics] Failed to init Physics Engine." << std::endl;
            return;
        }
        physics->setGroundPlaneEnabled(infiniteFloor);
    }

    if (assignedID >= nextID)
        nextID = assignedID + 1;
    Node *newNode = new Node({assignedID, name, type, parentNode, {}});
    nodes.push_back(newNode);
    nodeMap[newNode->ID] = newNode;
    parentNode->children.push_back(newNode);

    btRigidBody *body = physics->createBoxRigidBody(newNode->position, newNode->scale, mass);
    rigidBodies[newNode->ID] = body;
    
    // Save initial transform
    btTransform trans = body->getWorldTransform();
    initialTransforms[newNode->ID] = trans;

    if (forcedID == 0)
        initializeChildTransform(newNode);

    std::cout << "[SceneManager] Added new node with ID: " << newNode->ID << " and name: " << newNode->name << std::endl;
}

void SceneManager::addToParent(std::string &name, NodeType type, unsigned int parentID, unsigned int forcedID)
{
    unsigned int assignedID = (forcedID != 0) ? forcedID : findNextAvailableID();
    Node *parentNode = find_node(parentID);
    if (!parentNode)
    {
        std::cerr << "[SceneManager] Error: Parent node with ID " << parentID << " not found." << std::endl;
        return;
    }

    if (assignedID >= nextID)
        nextID = assignedID + 1;
    Node *newNode = new Node({assignedID, name, type, parentNode, {}});
    nodes.push_back(newNode);
    nodeMap[newNode->ID] = newNode;
    parentNode->children.push_back(newNode);

    if (forcedID == 0)
    {
        initializeChildTransform(newNode);
        
        extern Camera *globalCamera;
        if (type == NodeType::Camera && globalCamera)
        {
            if (newNode->parent)
            {
                glm::mat4 parentWorldMat = getWorldTransform(newNode->parent->ID);
                newNode->position = glm::vec3(glm::inverse(parentWorldMat) * glm::vec4(globalCamera->camPos, 1.0f));
            }
            else
            {
                newNode->position = globalCamera->camPos;
            }
            newNode->rotation = glm::vec3(glm::radians(globalCamera->pitch), glm::radians(globalCamera->yaw + 90.0f), 0.0f);
        }
    }

    std::cout << "[SceneManager] Added new node with ID: " << newNode->ID << " and name: " << newNode->name << std::endl;
}

void SceneManager::RenderModels(Shader &shader, float deltaTime)
{
    // 0. Kick off asynchronous physics simulation update
    if (simulate && !m_wasSimulating)
    {
        initialNodePositions.clear();
        initialNodeRotations.clear();
        initialNodeScales.clear();

        for (Node *n : nodes)
        {
            initialNodePositions[n->ID] = n->position;
            initialNodeRotations[n->ID] = n->rotation;
            initialNodeScales[n->ID] = n->scale;
        }

        // Wake up all rigid bodies when starting simulation
        for (auto &pair : rigidBodies)
        {
            if (pair.second)
                pair.second->activate(true);
        }

        m_wasSimulating = true;
    }
    else if (!simulate)
    {
        m_wasSimulating = false;
    }

    if (simulate && physics)
    {
        physicsCounter.store(1, std::memory_order_relaxed);
        Job physicsJob;
        physicsJob.completionCounter = &physicsCounter;
        physicsJob.work = [this, deltaTime]() {
            physics->update(deltaTime);
        };
        JobSystem::Get().Submit(physicsJob);
    }

    // 1. Parallel Animation Updates
    std::atomic<int> counter{0};
    for (auto &pair : models)
    {
        Model &model = pair.second;
        if (model.hasAnimation)
        {
            counter.fetch_add(1, std::memory_order_relaxed);
            Job job;
            job.completionCounter = &counter;
            job.work = [&model, deltaTime]() {
                model.UpdateAnimation(deltaTime);
            };
            JobSystem::Get().Submit(job);
        }
    }
    JobSystem::Get().Wait(&counter);

    // Wait for the background physics update job to complete before syncing and drawing
    if (simulate && physics)
    {
        JobSystem::Get().Wait(&physicsCounter);
    }

    // Synchronize transforms between physics engine and scene graph
    SyncTransforms();

    // 2. Set Light Uniforms (using synced transforms)
    int lightCount = lights.size();
    shader.setUniforms("numLights", (unsigned int)UniformType::Int, &lightCount);

    int idx = 0;
    for (auto &pair : lights)
    {
        Light &light = pair.second;
        const std::string &posName = (idx < 64) ? lightPosUniformNames[idx] : ("lightPositions[" + std::to_string(idx) + "]");
        const std::string &colName = (idx < 64) ? lightColUniformNames[idx] : ("lightColors[" + std::to_string(idx) + "]");

        glm::mat4 worldMat = getWorldTransform(light.ID);
        glm::vec3 worldPos = glm::vec3(worldMat[3]);

        shader.setUniforms(posName.c_str(), (unsigned int)UniformType::Vec3f, (void *)(glm::value_ptr(worldPos)));
        shader.setUniforms(colName.c_str(), (unsigned int)UniformType::Vec3f, (void *)(glm::value_ptr(light.color)));
        idx++;
    }

    for (auto &pair : models)
    {
        Model &model = pair.second;
        glm::mat4 modelMat = getWorldTransform(model.ID);
        shader.setUniforms("model", (unsigned int)UniformType::Mat4f, glm::value_ptr(modelMat));
        
        // Set material uniforms
        shader.setUniforms("material.ambient", static_cast<unsigned int>(UniformType::Vec3f), glm::value_ptr(model.material.ambient));
        shader.setUniforms("material.diffuse", static_cast<unsigned int>(UniformType::Vec3f), glm::value_ptr(model.material.diffuse));
        shader.setUniforms("material.specular", static_cast<unsigned int>(UniformType::Vec3f), glm::value_ptr(model.material.specular));
        shader.setUniforms("material.shininess", static_cast<unsigned int>(UniformType::Float), &model.material.shininess);

        model.Draw(shader);
    }
}

void SceneManager::RenderLights(Shader &shader)
{
    if (drawLights)
        for (auto &pair : lights)
        {
            Light &light = pair.second;
            shader.use();
            glm::mat4 worldMat = getWorldTransform(light.ID);
            worldMat = glm::scale(worldMat, glm::vec3(0.3f));
            shader.setUniforms("uLightColor", static_cast<unsigned int>(UniformType::Vec3f), (void *)(glm::value_ptr(light.color)));
            shader.setUniforms("model", static_cast<unsigned int>(UniformType::Mat4f), (void *)(glm::value_ptr(worldMat)));
            light.lightMesh.Draw(shader);
        }
}

void SceneManager::RenderParticles(float dt)
{
    for (auto &pair : particleEmitters)
    {
        ParticleEmitter &emitter = pair.second;
        glm::mat4 worldMat = getWorldTransform(emitter.ID);
        glm::vec3 worldPos = glm::vec3(worldMat[3]);

        for (int i = 0; i < 10; i++)
        {
            Particle newParticle;
            newParticle.Position = glm::vec3(0.0f, 0.0f, 0.0f);
            newParticle.Velocity = glm::vec3((rand() % 100 - 50) / 10.0f, 5.f, (rand() % 100 - 50) / 10.0f);
            newParticle.Life = 1.5f;
            newParticle.Color = emitter.Color;
            newParticle.Size = 0.05f;
            emitter.SpawnParticle(newParticle, worldPos);
        }

        emitter.Update(dt);
        emitter.Draw();
    }
}

void SceneManager::RenderPhysics(float dt, Shader &shader)
{
    // Wait for the background physics update job to complete before drawing
    if (simulate && physics)
    {
        JobSystem::Get().Wait(&physicsCounter);
    }

    if (drawPhysics && physics)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        physics->Draw(shader);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
}

void SceneManager::deleteNode(unsigned int ID)
{
    if (ID == root->ID)
    {
        std::cerr << "[SceneManager] Cannot delete root node." << std::endl;
        return;
    }

    if (ID == activeCameraID)
    {
        activeCameraID = 0;
        cameraChangesPending = false;
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

    // Delete all children recursively
    while (!nodeToDelete->children.empty())
    {
        deleteNode(nodeToDelete->children.back()->ID);
    }

    if (nodeToDelete->type == NodeType::Model)
    {
        auto it = models.find(nodeToDelete->ID);
        if (it != models.end())
        {
            std::cout << "[SceneManager] Deleting model with ID: " << it->second.ID << " and path: " << it->second.directory << std::endl;
            models.erase(it);
        }
        else
        {
            std::cerr << "[SceneManager] Warning: No model found for node ID " << nodeToDelete->ID << std::endl;
        }
    }

    if (nodeToDelete->type == NodeType::Light)
    {
        auto it = lights.find(nodeToDelete->ID);
        if (it != lights.end())
        {
            std::cout << "[SceneManager] Deleting Light with ID: " << it->second.ID << std::endl;
            lights.erase(it);
        }
        else
            std::cerr << "[SceneManager] Warning: No Light found for node ID " << nodeToDelete->ID << std::endl;
    }
    if (nodeToDelete->type == NodeType::Particles)
    {
        auto it = particleEmitters.find(nodeToDelete->ID);
        if (it != particleEmitters.end())
        {
            std::cout << "[SceneManager] Deleting Particle Emitter with ID: " << it->second.ID << std::endl;
            // deleteNode(nodeToDelete->children[0]->ID);
            particleEmitters.erase(it);
        }
        else
            std::cerr << "[SceneManager] Warning: No Particle Emitter found for node ID " << nodeToDelete->ID << std::endl;
    }
    if (nodeToDelete->type == NodeType::RigidBody)
    {
        btRigidBody *body = getRigidBodyByID(nodeToDelete->ID);
        if (!body)
        {
            std::cerr << "[SceneManager] Warning: No Rigid Body found for node ID " << nodeToDelete->ID << std::endl;
            return;
        }

        physics->deleteRigidBody(body);
        rigidBodies.erase(nodeToDelete->ID);
    }

    std::cout << "[SceneManager] Deleting node with ID: " << nodeToDelete->ID << " and name: " << nodeToDelete->name << std::endl;
    nodes.erase(std::remove(nodes.begin(), nodes.end(), nodeToDelete), nodes.end());
    nodeMap.erase(ID);
    delete nodeToDelete;
}

Model *SceneManager::getModelByID(unsigned int ID)
{
    auto it = models.find(ID);
    if (it != models.end())
        return &it->second;
    return nullptr;
}

Light *SceneManager::getLightByID(unsigned int ID)
{
    auto it = lights.find(ID);
    if (it != lights.end())
        return &it->second;
    return nullptr;
}

ParticleEmitter *SceneManager::getEmitterByID(unsigned int ID)
{
    auto it = particleEmitters.find(ID);
    if (it != particleEmitters.end())
        return &it->second;
    return nullptr;
}

btRigidBody *SceneManager::getRigidBodyByID(unsigned int ID)
{
    auto it = rigidBodies.find(ID);
    if (it != rigidBodies.end())
        return it->second;
    return nullptr;
}

json SceneManager::serializeScene()
{
    if (!root)
    {
        return json::object();
    }

    json outJson;
    outJson["projectName"] = projectName;
    outJson["activeCameraID"] = activeCameraID;

    std::function<json(Node *)> buildNodeJson = [&](Node *node) -> json
    {
        json j;
        j["id"] = node->ID;
        j["name"] = node->name;
        j["type"] = nodeTypeToString(node->type);

        // Save model-specific data
        if (node->type == NodeType::Model)
        {
            auto it = models.find(node->ID);

            if (it != models.end())
            {
                Model &model = it->second;
                if (!model.directory.empty())
                {
                    j["modelPath"] = model.directory;
                }
                j["position"] = {model.getPosition().x, model.getPosition().y, model.getPosition().z};
                j["rotation"] = {model.getRotation().x, model.getRotation().y, model.getRotation().z};
                j["scale"] = {model.getScale().x, model.getScale().y, model.getScale().z};
                
                // Save material data
                j["material"] = {
                    {"ambient", {model.material.ambient.x, model.material.ambient.y, model.material.ambient.z}},
                    {"diffuse", {model.material.diffuse.x, model.material.diffuse.y, model.material.diffuse.z}},
                    {"specular", {model.material.specular.x, model.material.specular.y, model.material.specular.z}},
                    {"shininess", model.material.shininess}
                };
            }
        }

        // Save light-specific data
        else if (node->type == NodeType::Light)
        {
            auto it = lights.find(node->ID);

            if (it != lights.end())
            {
                Light &light = it->second;
                j["color"] = {light.color.x, light.color.y, light.color.z};
                j["position"] = {light.position.x, light.position.y, light.position.z};
            }
        }

        else if (node->type == NodeType::Particles)
        {
            auto it = particleEmitters.find(node->ID);

            if (it != particleEmitters.end())
            {
                ParticleEmitter &emitter = it->second;
                j["color"] = {emitter.Color.r, emitter.Color.g, emitter.Color.b, emitter.Color.a};
                j["position"] = {emitter.Position.x, emitter.Position.y, emitter.Position.z};
                j["shaderName"] = emitter.shader ? emitter.shader->Name : "";
                j["maxParticles"] = emitter.maxParticles;
            }
        }
        else if (node->type == NodeType::RigidBody)
        {
            auto it = rigidBodies.find(node->ID);
            if (it != rigidBodies.end())
            {
                btRigidBody *body = it->second;
                float mass = (body->getInvMass() > 0.0f) ? (1.0f / body->getInvMass()) : 0.0f;
                j["mass"] = mass;
                j["shape"] = "CUBE";
                
                btTransform trans;
                if (body->getMotionState())
                    body->getMotionState()->getWorldTransform(trans);
                else
                    trans = body->getWorldTransform();
                    
                j["position"] = {trans.getOrigin().getX(), trans.getOrigin().getY(), trans.getOrigin().getZ()};
            }
        }

        // For other types (Empty, Camera, Root), save the node transform properties directly
        if (node->type == NodeType::Empty || node->type == NodeType::Camera || node->type == NodeType::Root)
        {
            j["position"] = {node->position.x, node->position.y, node->position.z};
            j["rotation"] = {node->rotation.x, node->rotation.y, node->rotation.z};
            j["scale"] = {node->scale.x, node->scale.y, node->scale.z};
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
    return outJson;
}

void SceneManager::deserializeScene(const nlohmann::json &data)
{
    projectName = data.value("projectName", "UnnamedProject");
    activeCameraID = data.value("activeCameraID", 0u);

    // Cleanup
    for (Node *node : nodes)
    {
        delete node;
    }
    nodes.clear();
    nodeMap.clear();
    models.clear();
    lights.clear();
    particleEmitters.clear();
    
    if (physics)
    {
        delete physics;
        physics = nullptr;
    }
    rigidBodies.clear();
    initialTransforms.clear();
    
    nextID = 1;

    std::function<void(const json &, Node *)> buildNodeRecursive = [&](const json &j, Node *parent)
    {
        unsigned int id = j["id"];
        std::string name = j["name"];
        NodeType type = stringToNodeType(j["type"]);

        if (type == NodeType::Model && j.contains("modelPath"))
        {
            std::string modelPath = j["modelPath"];
            addToParent(name, modelPath, type, parent->ID, id);

            // Set model transform if available
            auto *model = getModelByID(id);
            Node *newNode = find_node(id);
            if (model && newNode)
            {
                if (j.contains("position"))
                {
                    glm::vec3 pos(j["position"][0], j["position"][1], j["position"][2]);
                    model->setPosition(pos);
                    newNode->position = pos;
                }
                if (j.contains("rotation"))
                {
                    glm::vec3 rot(j["rotation"][0], j["rotation"][1], j["rotation"][2]);
                    model->setRotation(rot);
                    newNode->rotation = rot;
                }
                if (j.contains("scale"))
                {
                    glm::vec3 scl(j["scale"][0], j["scale"][1], j["scale"][2]);
                    model->setScale(scl);
                    newNode->scale = scl;
                }
                
                // Load material data
                if (j.contains("material"))
                {
                    auto &matJ = j["material"];
                    if (matJ.contains("ambient"))
                        model->material.ambient = glm::vec3(matJ["ambient"][0], matJ["ambient"][1], matJ["ambient"][2]);
                    if (matJ.contains("diffuse"))
                        model->material.diffuse = glm::vec3(matJ["diffuse"][0], matJ["diffuse"][1], matJ["diffuse"][2]);
                    if (matJ.contains("specular"))
                        model->material.specular = glm::vec3(matJ["specular"][0], matJ["specular"][1], matJ["specular"][2]);
                    if (matJ.contains("shininess"))
                        model->material.shininess = matJ["shininess"];
                }
            }
        }
        else if (type == NodeType::Light)
        {
            if (parent->type == NodeType::Particles)
                return;
            addToParent(name, type, parent->ID, LightType::DIRECTIONAL, id);

            // Set light data if available
            auto *light = getLightByID(id);
            Node *newNode = find_node(id);
            if (light && newNode)
            {
                if (j.contains("position"))
                {
                    glm::vec3 pos(j["position"][0], j["position"][1], j["position"][2]);
                    light->position = pos;
                    newNode->position = pos;
                }
                if (j.contains("color"))
                    light->color = glm::vec3(j["color"][0], j["color"][1], j["color"][2]);
            }
        }
        else if (type == NodeType::Particles)
        {
            std::string shaderName;
            unsigned int maxParts = 0;

            if (j.contains("shaderName"))
                shaderName = j["shaderName"];
            if (j.contains("maxParticles"))
                maxParts = j["maxParticles"];

            addToParent(name, type, parent->ID, shaderName, maxParts, id);

            auto *emitter = getEmitterByID(id);
            Node *newNode = find_node(id);
            if (emitter && newNode)
            {
                if (j.contains("position"))
                {
                    glm::vec3 pos(j["position"][0], j["position"][1], j["position"][2]);
                    emitter->Position = pos;
                    newNode->position = pos;
                }
                if (j.contains("color"))
                    emitter->Color = glm::vec4(j["color"][0], j["color"][1], j["color"][2], j["color"][3]);
            }
        }
        else if (type == NodeType::RigidBody)
        {
            float mass = 1.0f;
            if (j.contains("mass"))
                mass = j["mass"];
            addToParent(name, type, parent->ID, RigidBodyShape::CUBE, mass, id);

            // Set physics rigid body position if available
            auto *body = getRigidBodyByID(id);
            Node *newNode = find_node(id);
            if (body && newNode && j.contains("position"))
            {
                btTransform trans = body->getWorldTransform();
                trans.setOrigin(btVector3(j["position"][0], j["position"][1], j["position"][2]));
                body->setWorldTransform(trans);
                if (body->getMotionState())
                {
                    body->getMotionState()->setWorldTransform(trans);
                }
                
                // Save loaded position as the initial transform
                initialTransforms[id] = trans;
                newNode->position = glm::vec3(j["position"][0], j["position"][1], j["position"][2]);
            }
        }
        else
        {
            addToParent(name, type, parent->ID, id);
            Node *newNode = find_node(id);
            if (newNode)
            {
                if (j.contains("position"))
                    newNode->position = glm::vec3(j["position"][0], j["position"][1], j["position"][2]);
                if (j.contains("rotation"))
                    newNode->rotation = glm::vec3(j["rotation"][0], j["rotation"][1], j["rotation"][2]);
                if (j.contains("scale"))
                    newNode->scale = glm::vec3(j["scale"][0], j["scale"][1], j["scale"][2]);
            }
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
    nodeMap[root->ID] = root;
    nextID = std::max(nextID, rootID + 1);

    if (rootJson.contains("children"))
    {
        for (auto &childJson : rootJson["children"])
        {
            buildNodeRecursive(childJson, root);
        }
    }
}

void SceneManager::pushUndoState()
{
    undoStack.push_back(serializeScene());
    if (undoStack.size() > 50)
    {
        undoStack.erase(undoStack.begin());
    }
    redoStack.clear();
    isDirty = true;
}

void SceneManager::undo()
{
    if (undoStack.empty())
        return;

    redoStack.push_back(serializeScene());
    json previousState = undoStack.back();
    undoStack.pop_back();

    deserializeScene(previousState);
    isDirty = true;
}

void SceneManager::redo()
{
    if (redoStack.empty())
        return;

    undoStack.push_back(serializeScene());
    json nextState = redoStack.back();
    redoStack.pop_back();

    deserializeScene(nextState);
    isDirty = true;
}

void SceneManager::saveScene()
{
    json outJson = serializeScene();

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
        isDirty = false;
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

    deserializeScene(data);
    
    isDirty = false;
    undoStack.clear();
    redoStack.clear();
    std::cout << "[SceneManager] Scene loaded successfully." << std::endl;
}

Node *SceneManager::getCameraNode()
{
    for (Node *n : nodes)
    {
        if (n->type == NodeType::Camera)
            return n;
    }
    return nullptr;
}

unsigned int SceneManager::findNextAvailableID()
{
    unsigned int id = 1;
    while (nodeMap.find(id) != nodeMap.end())
    {
        id++;
    }
    return id;
}

Node *SceneManager::find_node(unsigned int id)
{
    auto it = nodeMap.find(id);
    if (it != nodeMap.end())
        return it->second;
    return nullptr;
}

glm::mat4 SceneManager::getWorldTransform(unsigned int id)
{
    Node *node = find_node(id);
    if (!node)
        return glm::mat4(1.0f);

    std::vector<Node *> path;
    Node *curr = node;
    while (curr)
    {
        path.push_back(curr);
        curr = curr->parent;
    }

    glm::mat4 worldMat = glm::mat4(1.0f);
    for (auto it = path.rbegin(); it != path.rend(); ++it)
    {
        Node *n = *it;
        glm::mat4 localMat = glm::mat4(1.0f);
        if (n->type == NodeType::Root)
        {
            localMat = glm::mat4(1.0f);
        }
        else if (n->type == NodeType::Model)
        {
            Model *model = getModelByID(n->ID);
            if (model)
                localMat = model->getModelMatrix();
        }
        else if (n->type == NodeType::Light)
        {
            Light *light = getLightByID(n->ID);
            if (light)
            {
                localMat = glm::translate(glm::mat4(1.0f), light->position);
                localMat = glm::rotate(localMat, n->rotation.x, glm::vec3(1.f, 0.f, 0.f));
                localMat = glm::rotate(localMat, n->rotation.y, glm::vec3(0.f, 1.f, 0.f));
                localMat = glm::rotate(localMat, n->rotation.z, glm::vec3(0.f, 0.f, 1.f));
                localMat = glm::scale(localMat, n->scale);
            }
        }
        else if (n->type == NodeType::Particles)
        {
            ParticleEmitter *emitter = getEmitterByID(n->ID);
            if (emitter)
            {
                localMat = glm::translate(glm::mat4(1.0f), emitter->Position);
                localMat = glm::rotate(localMat, n->rotation.x, glm::vec3(1.f, 0.f, 0.f));
                localMat = glm::rotate(localMat, n->rotation.y, glm::vec3(0.f, 1.f, 0.f));
                localMat = glm::rotate(localMat, n->rotation.z, glm::vec3(0.f, 0.f, 1.f));
                localMat = glm::scale(localMat, n->scale);
            }
        }
        else if (n->type == NodeType::RigidBody)
        {
            btRigidBody *body = getRigidBodyByID(n->ID);
            if (body)
            {
                btTransform trans;
                if (body->getMotionState())
                    body->getMotionState()->getWorldTransform(trans);
                else
                    trans = body->getWorldTransform();
                
                float m[16];
                trans.getOpenGLMatrix(m);
                localMat = glm::make_mat4(m);
                worldMat = localMat; // Override accumulated transform because Bullet body transform is in world space
                continue; // Skip the multiplication at the bottom
            }
            else
            {
                localMat = glm::translate(glm::mat4(1.0f), n->position);
                localMat = glm::rotate(localMat, n->rotation.x, glm::vec3(1.f, 0.f, 0.f));
                localMat = glm::rotate(localMat, n->rotation.y, glm::vec3(0.f, 1.f, 0.f));
                localMat = glm::rotate(localMat, n->rotation.z, glm::vec3(0.f, 0.f, 1.f));
                localMat = glm::scale(localMat, n->scale);
            }
        }
        else
        {
            localMat = glm::translate(glm::mat4(1.0f), n->position);
            localMat = glm::rotate(localMat, n->rotation.x, glm::vec3(1.f, 0.f, 0.f));
            localMat = glm::rotate(localMat, n->rotation.y, glm::vec3(0.f, 1.f, 0.f));
            localMat = glm::rotate(localMat, n->rotation.z, glm::vec3(0.f, 0.f, 1.f));
            localMat = glm::scale(localMat, n->scale);
        }
        
        worldMat = worldMat * localMat;
    }
    return worldMat;
}

void SceneManager::SyncTransforms()
{
    for (Node *n : nodes)
    {
        if (n->type == NodeType::RigidBody)
        {
            btRigidBody *body = getRigidBodyByID(n->ID);
            if (!body) continue;

            if (simulate)
            {
                // Sync Bullet -> Scene Graph Node (Simulation Mode)
                btTransform trans;
                if (body->getMotionState())
                    body->getMotionState()->getWorldTransform(trans);
                else
                    trans = body->getWorldTransform();

                float m[16];
                trans.getOpenGLMatrix(m);
                glm::mat4 worldMat = glm::make_mat4(m);

                // Update the RigidBody node's local transform relative to its parent
                glm::mat4 parentWorldMat = glm::mat4(1.0f);
                if (n->parent)
                    parentWorldMat = getWorldTransform(n->parent->ID);

                glm::mat4 invParent = glm::inverse(parentWorldMat);
                glm::mat4 localMat = invParent * worldMat;

                // Extract position
                n->position = glm::vec3(localMat[3]);

                // Extract rotation
                glm::vec3 scale;
                scale.x = glm::length(glm::vec3(localMat[0]));
                scale.y = glm::length(glm::vec3(localMat[1]));
                scale.z = glm::length(glm::vec3(localMat[2]));

                glm::mat3 rotMat;
                rotMat[0] = (scale.x > 0.0f) ? (glm::vec3(localMat[0]) / scale.x) : glm::vec3(1, 0, 0);
                rotMat[1] = (scale.y > 0.0f) ? (glm::vec3(localMat[1]) / scale.y) : glm::vec3(0, 1, 0);
                rotMat[2] = (scale.z > 0.0f) ? (glm::vec3(localMat[2]) / scale.z) : glm::vec3(0, 0, 1);
                n->rotation = extractXYZ(rotMat);
            }
            else
            {
                // Sync Scene Graph -> Bullet (Editor Mode)
                glm::mat4 parentWorldMat = glm::mat4(1.0f);
                if (n->parent)
                    parentWorldMat = getWorldTransform(n->parent->ID);

                glm::mat4 localMat = glm::translate(glm::mat4(1.0f), n->position);
                localMat = glm::rotate(localMat, n->rotation.x, glm::vec3(1.f, 0.f, 0.f));
                localMat = glm::rotate(localMat, n->rotation.y, glm::vec3(0.f, 1.f, 0.f));
                localMat = glm::rotate(localMat, n->rotation.z, glm::vec3(0.f, 0.f, 1.f));
                localMat = glm::scale(localMat, n->scale);

                glm::mat4 worldMat = parentWorldMat * localMat;

                glm::vec3 worldScale;
                worldScale.x = glm::length(glm::vec3(worldMat[0]));
                worldScale.y = glm::length(glm::vec3(worldMat[1]));
                worldScale.z = glm::length(glm::vec3(worldMat[2]));

                // Normalize rotation basis vectors to prevent scale corruption of btTransform
                glm::mat4 normWorldMat = worldMat;
                if (worldScale.x > 0.0f) normWorldMat[0] = worldMat[0] / worldScale.x;
                if (worldScale.y > 0.0f) normWorldMat[1] = worldMat[1] / worldScale.y;
                if (worldScale.z > 0.0f) normWorldMat[2] = worldMat[2] / worldScale.z;

                btTransform trans;
                trans.setFromOpenGLMatrix(glm::value_ptr(normWorldMat));
                body->setWorldTransform(trans);
                if (body->getMotionState())
                    body->getMotionState()->setWorldTransform(trans);

                body->getCollisionShape()->setLocalScaling(btVector3(worldScale.x, worldScale.y, worldScale.z));
                physics->getDynamicsWorld()->updateSingleAabb(body);

                // Recompute local inertia and update mass props
                float mass = (body->getInvMass() == 0.0f) ? 0.0f : 1.0f / body->getInvMass();
                btVector3 localInertia(0, 0, 0);
                if (mass > 0.0f)
                    body->getCollisionShape()->calculateLocalInertia(mass, localInertia);
                body->setMassProps(mass, localInertia);
                body->updateInertiaTensor();
                
                // Keep initial transforms up to date for reset
                initialTransforms[n->ID] = trans;
            }
        }
    }
}

void SceneManager::ResetPhysics()
{
    // 1. Restore all scene graph nodes to their saved pre-simulation transforms
    for (Node *n : nodes)
    {
        auto posIt = initialNodePositions.find(n->ID);
        if (posIt != initialNodePositions.end())
        {
            n->position = posIt->second;
        }
        auto rotIt = initialNodeRotations.find(n->ID);
        if (rotIt != initialNodeRotations.end())
        {
            n->rotation = rotIt->second;
        }
        auto sclIt = initialNodeScales.find(n->ID);
        if (sclIt != initialNodeScales.end())
        {
            n->scale = sclIt->second;
        }

        // Sync back to component states
        if (n->type == NodeType::Model)
        {
            Model *model = getModelByID(n->ID);
            if (model)
            {
                model->setPosition(n->position);
                model->setRotation(n->rotation);
                model->setScale(n->scale);
            }
        }
        else if (n->type == NodeType::Light)
        {
            Light *light = getLightByID(n->ID);
            if (light)
            {
                light->position = n->position;
            }
        }
        else if (n->type == NodeType::Particles)
        {
            ParticleEmitter *emitter = getEmitterByID(n->ID);
            if (emitter)
            {
                emitter->Position = n->position;
            }
        }
    }

    // 2. Restore all Bullet rigid bodies to their initial transforms
    for (auto &pair : rigidBodies)
    {
        unsigned int id = pair.first;
        btRigidBody *body = pair.second;
        if (body)
        {
            // Clear velocities and forces
            body->setLinearVelocity(btVector3(0, 0, 0));
            body->setAngularVelocity(btVector3(0, 0, 0));
            body->clearForces();

            // Reset transform to initial state
            auto it = initialTransforms.find(id);
            if (it != initialTransforms.end())
            {
                body->setWorldTransform(it->second);
                if (body->getMotionState())
                {
                    body->getMotionState()->setWorldTransform(it->second);
                }
            }

            // Wake up rigid body on reset
            body->activate(true);
        }
    }
    std::cout << "[Physics] Physics world reset to initial positions." << std::endl;
}

void SceneManager::initializeChildTransform(Node *newNode)
{
    if (newNode && newNode->parent)
    {
        glm::mat4 parentWorldMat = getWorldTransform(newNode->parent->ID);
        glm::mat4 invParent = glm::inverse(parentWorldMat);
        
        // Extract translation
        newNode->position = glm::vec3(invParent[3]);
        
        // Extract scale
        glm::vec3 scale;
        scale.x = glm::length(glm::vec3(invParent[0]));
        scale.y = glm::length(glm::vec3(invParent[1]));
        scale.z = glm::length(glm::vec3(invParent[2]));
        newNode->scale = scale;
        
        // Extract rotation euler angles
        glm::mat3 rotMat;
        rotMat[0] = (scale.x > 0.0f) ? (glm::vec3(invParent[0]) / scale.x) : glm::vec3(1, 0, 0);
        rotMat[1] = (scale.y > 0.0f) ? (glm::vec3(invParent[1]) / scale.y) : glm::vec3(0, 1, 0);
        rotMat[2] = (scale.z > 0.0f) ? (glm::vec3(invParent[2]) / scale.z) : glm::vec3(0, 0, 1);
        newNode->rotation = extractXYZ(rotMat);
        
        // Sync to component
        if (newNode->type == NodeType::Model)
        {
            Model *model = getModelByID(newNode->ID);
            if (model)
            {
                model->setPosition(newNode->position);
                model->setRotation(newNode->rotation);
                model->setScale(newNode->scale);
            }
        }
        else if (newNode->type == NodeType::Light)
        {
            Light *light = getLightByID(newNode->ID);
            if (light)
            {
                light->position = newNode->position;
            }
        }
        else if (newNode->type == NodeType::Particles)
        {
            ParticleEmitter *emitter = getEmitterByID(newNode->ID);
            if (emitter)
            {
                emitter->Position = newNode->position;
            }
        }
        else if (newNode->type == NodeType::RigidBody)
        {
            btRigidBody *body = getRigidBodyByID(newNode->ID);
            if (body)
            {
                btTransform trans = body->getWorldTransform();
                trans.setOrigin(btVector3(newNode->position.x, newNode->position.y, newNode->position.z));
                glm::quat q(newNode->rotation);
                trans.setRotation(btQuaternion(q.x, q.y, q.z, q.w));
                body->setWorldTransform(trans);
                if (body->getMotionState())
                    body->getMotionState()->setWorldTransform(trans);
                initialTransforms[newNode->ID] = trans;
                
                body->getCollisionShape()->setLocalScaling(btVector3(newNode->scale.x, newNode->scale.y, newNode->scale.z));
                physics->getDynamicsWorld()->updateSingleAabb(body);
            }
        }
    }
}