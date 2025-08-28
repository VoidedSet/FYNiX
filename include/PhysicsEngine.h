#pragma once

#include "btBulletDynamicsCommon.h"
#include <vector>

#include "Mesh.h"

class PhysicsEngine
{
public:
    bool drawDebugMesh = true;
    Mesh debugMesh = Mesh(MeshType::CUBE);

    PhysicsEngine();
    ~PhysicsEngine();

    void update(float deltaTime);
    void Draw(Shader &shader);

    btDiscreteDynamicsWorld *getDynamicsWorld();

    void createBoxRigidBody(glm::vec3 position, glm::vec3 size, float mass);

private:
    btDefaultCollisionConfiguration *m_collisionConfiguration;
    btCollisionDispatcher *m_dispatcher;
    btBroadphaseInterface *m_overlappingPairCache;
    btSequentialImpulseConstraintSolver *m_solver;
    btDiscreteDynamicsWorld *m_dynamicsWorld;

    std::vector<btCollisionShape *> m_collisionShapes;
};