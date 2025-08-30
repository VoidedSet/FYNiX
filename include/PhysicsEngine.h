#pragma once

#include "btBulletDynamicsCommon.h"
#include <vector>

#include "Mesh.h"

// only cube operational
enum class RigidBodyShape
{
    CUBE,
    CAPSULE,
    SPHERE
};

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

    btRigidBody *createBoxRigidBody(glm::vec3 position, glm::vec3 size, float mass);

    void setGravity(int gravity) { m_dynamicsWorld->setGravity(btVector3(0, -gravity, 0)); }

private:
    btDefaultCollisionConfiguration *m_collisionConfiguration;
    btCollisionDispatcher *m_dispatcher;
    btBroadphaseInterface *m_overlappingPairCache;
    btSequentialImpulseConstraintSolver *m_solver;
    btDiscreteDynamicsWorld *m_dynamicsWorld;

    std::vector<btCollisionShape *> m_collisionShapes;
};