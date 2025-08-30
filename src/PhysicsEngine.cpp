#include "PhysicsEngine.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>

PhysicsEngine::PhysicsEngine()
{
    m_collisionConfiguration = new btDefaultCollisionConfiguration();

    m_dispatcher = new btCollisionDispatcher(m_collisionConfiguration);

    m_overlappingPairCache = new btDbvtBroadphase();

    m_solver = new btSequentialImpulseConstraintSolver();

    m_dynamicsWorld = new btDiscreteDynamicsWorld(m_dispatcher, m_overlappingPairCache, m_solver, m_collisionConfiguration);

    m_dynamicsWorld->setGravity(btVector3(0, -10, 0));

    std::cout << "[Physics] Physics Engine Initialized" << std::endl;
}

PhysicsEngine::~PhysicsEngine()
{
    for (int i = m_dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--)
    {
        btCollisionObject *obj = m_dynamicsWorld->getCollisionObjectArray()[i];
        btRigidBody *body = btRigidBody::upcast(obj);
        if (body && body->getMotionState())
        {
            delete body->getMotionState();
        }
        m_dynamicsWorld->removeCollisionObject(obj);
        delete obj;
    }

    for (int i = 0; i < m_collisionShapes.size(); i++)
    {
        btCollisionShape *shape = m_collisionShapes[i];
        delete shape;
    }
    m_collisionShapes.clear();

    delete m_dynamicsWorld;
    delete m_solver;
    delete m_overlappingPairCache;
    delete m_dispatcher;
    delete m_collisionConfiguration;

    std::cout << "[Physics] Physics Engine Destroyed" << std::endl;
}

void PhysicsEngine::update(float deltaTime)
{
    // stepSimulation(timeStep, maxSubSteps)
    // timeStep: The amount of time to simulate, in seconds.
    // maxSubSteps: To ensure simulation accuracy, Bullet can perform smaller internal steps.
    // 10 is a good default value.
    m_dynamicsWorld->stepSimulation(deltaTime, 10);
}

void PhysicsEngine::Draw(Shader &shader)
{
    if (!drawDebugMesh)
        return;

    shader.use();

    btCollisionObjectArray &objects = m_dynamicsWorld->getCollisionObjectArray();

    for (int i = 0; i < m_dynamicsWorld->getNumCollisionObjects(); i++)
    {
        btCollisionObject *obj = objects[i];
        btRigidBody *body = btRigidBody::upcast(obj);

        if (body && body->getMotionState())
        {
            btTransform trans;
            body->getMotionState()->getWorldTransform(trans);

            glm::vec3 scale(1.0f, 1.0f, 1.0f);
            btCollisionShape *shape = body->getCollisionShape();

            if (shape->getShapeType() == BOX_SHAPE_PROXYTYPE)
            {
                btBoxShape *boxShape = static_cast<btBoxShape *>(shape);
                btVector3 halfExtents = boxShape->getHalfExtentsWithMargin();
                scale = glm::vec3(halfExtents.x() * 2.0f, halfExtents.y() * 2.0f, halfExtents.z() * 2.0f);
            }

            glm::mat4 modelMatrix;
            trans.getOpenGLMatrix(glm::value_ptr(modelMatrix));

            modelMatrix = glm::scale(modelMatrix, scale);

            shader.setUniforms("uLightColor", static_cast<unsigned int>(UniformType::Vec3f), (void *)(glm::value_ptr(glm::vec3(1.f))));
            shader.setUniforms("model", static_cast<unsigned int>(UniformType::Mat4f), (void *)(glm::value_ptr(modelMatrix)));

            debugMesh.Draw(shader);
        }
    }
}

btDiscreteDynamicsWorld *PhysicsEngine::getDynamicsWorld()
{
    return m_dynamicsWorld;
}

btRigidBody *PhysicsEngine::createBoxRigidBody(glm::vec3 position, glm::vec3 size, float mass)
{
    btCollisionShape *boxShape = new btBoxShape(btVector3(size.x * 0.5f, size.y * 0.5f, size.z * 0.5f));
    m_collisionShapes.push_back(boxShape);

    btTransform startTransform;
    startTransform.setIdentity();
    startTransform.setOrigin(btVector3(position.x, position.y, position.z));

    btVector3 localInertia(0, 0, 0);
    if (mass != 0.0f)
        boxShape->calculateLocalInertia(mass, localInertia);

    btDefaultMotionState *myMotionState = new btDefaultMotionState(startTransform);

    btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, boxShape, localInertia);
    btRigidBody *body = new btRigidBody(rbInfo);

    m_dynamicsWorld->addRigidBody(body);

    std::cout << "[Physics] Created a Rigid Body!" << std::endl;

    return body;
}
