#include "PhysicsEngine.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/constants.hpp>

#include <iostream>

namespace
{
    Mesh CreateSphereMesh(float radius, unsigned int rings = 16, unsigned int sectors = 16)
    {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;

        float const R = 1.0f / (float)(rings - 1);
        float const S = 1.0f / (float)(sectors - 1);

        for (unsigned int r = 0; r < rings; ++r) {
            for (unsigned int s = 0; s < sectors; ++s) {
                float const y = sin(-glm::half_pi<float>() + glm::pi<float>() * r * R);
                float const x = cos(2 * glm::pi<float>() * s * S) * sin(glm::pi<float>() * r * R);
                float const z = sin(2 * glm::pi<float>() * s * S) * sin(glm::pi<float>() * r * R);

                Vertex v;
                v.postition = glm::vec3(x, y, z) * radius;
                v.normal = glm::vec3(x, y, z);
                v.texCoords = glm::vec2(s * S, r * R);
                vertices.push_back(v);
            }
        }

        for (unsigned int r = 0; r < rings - 1; ++r) {
            for (unsigned int s = 0; s < sectors - 1; ++s) {
                indices.push_back(r * sectors + s);
                indices.push_back(r * sectors + (s + 1));
                indices.push_back((r + 1) * sectors + (s + 1));

                indices.push_back(r * sectors + s);
                indices.push_back((r + 1) * sectors + (s + 1));
                indices.push_back((r + 1) * sectors + s);
            }
        }

        return Mesh(vertices, indices, {});
    }

    Mesh CreateCapsuleMesh(float radius, float height, unsigned int subdivisions = 16)
    {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;

        unsigned int rings = subdivisions / 2;
        unsigned int sectors = subdivisions;

        for (unsigned int r = 0; r <= rings; ++r) {
            float theta = -glm::half_pi<float>() + glm::pi<float>() * r / rings;
            float y = sin(theta) * radius;
            float cosTheta = cos(theta);

            if (theta > 0.0f) {
                y += height * 0.5f;
            } else {
                y -= height * 0.5f;
            }

            for (unsigned int s = 0; s <= sectors; ++s) {
                float phi = 2 * glm::pi<float>() * s / sectors;
                float x = cos(phi) * cosTheta * radius;
                float z = sin(phi) * cosTheta * radius;

                Vertex v;
                v.postition = glm::vec3(x, y, z);
                v.normal = glm::normalize(glm::vec3(x, theta > 0.0f ? y - height * 0.5f : y + height * 0.5f, z));
                v.texCoords = glm::vec2((float)s / sectors, (float)r / rings);
                vertices.push_back(v);
            }
        }

        for (unsigned int r = 0; r < rings; ++r) {
            for (unsigned int s = 0; s < sectors; ++s) {
                unsigned int current = r * (sectors + 1) + s;
                unsigned int next = current + 1;
                unsigned int bottom = current + (sectors + 1);
                unsigned int bottomNext = bottom + 1;

                indices.push_back(current);
                indices.push_back(next);
                indices.push_back(bottomNext);

                indices.push_back(current);
                indices.push_back(bottomNext);
                indices.push_back(bottom);
            }
        }

        return Mesh(vertices, indices, {});
    }
}


PhysicsEngine::PhysicsEngine()
{
    m_collisionConfiguration = new btDefaultCollisionConfiguration();

    m_dispatcher = new btCollisionDispatcher(m_collisionConfiguration);

    m_overlappingPairCache = new btDbvtBroadphase();

    m_solver = new btSequentialImpulseConstraintSolver();

    m_dynamicsWorld = new btDiscreteDynamicsWorld(m_dispatcher, m_overlappingPairCache, m_solver, m_collisionConfiguration);

    m_dynamicsWorld->setGravity(btVector3(0, -10, 0));

    m_groundBody = nullptr;
    setGroundPlaneEnabled(true);

    std::cout << "[Physics] Physics Engine Initialized" << std::endl;
}

PhysicsEngine::~PhysicsEngine()
{
    setGroundPlaneEnabled(false);

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
    float clampedDelta = std::min(deltaTime, 0.1f);
    m_dynamicsWorld->stepSimulation(clampedDelta, 10);
}

void PhysicsEngine::Draw(Shader &shader)
{
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
            else if (shape->getShapeType() == STATIC_PLANE_PROXYTYPE)
            {
                scale = glm::vec3(100.0f, 0.01f, 100.0f);
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

void PhysicsEngine::deleteRigidBody(btRigidBody *body)
{
    if (!body)
    {
        return;
    }

    // 1. Remove the rigid body from the dynamics world.
    m_dynamicsWorld->removeRigidBody(body);

    // 2. Delete the motion state.
    if (body->getMotionState())
    {
        delete body->getMotionState();
    }

    // 3. Delete the collision shape.
    btCollisionShape *shape = body->getCollisionShape();
    if (shape)
    {
        // Remove the shape from our tracking vector.
        // The erase-remove idiom is a clean way to do this.
        m_collisionShapes.erase(std::remove(m_collisionShapes.begin(), m_collisionShapes.end(), shape), m_collisionShapes.end());
        delete shape;
    }

    // 4. Finally, delete the rigid body itself.
    delete body;

    std::cout << "[Physics] Deleted a Rigid Body!" << std::endl;
}

void PhysicsEngine::setGroundPlaneEnabled(bool enabled)
{
    if (enabled)
    {
        if (!m_groundBody)
        {
            btCollisionShape *groundShape = new btStaticPlaneShape(btVector3(0.0f, 1.0f, 0.0f), 0.0f);
            m_collisionShapes.push_back(groundShape);

            btTransform groundTransform;
            groundTransform.setIdentity();
            groundTransform.setOrigin(btVector3(0.0f, 0.0f, 0.0f));

            btDefaultMotionState *myMotionState = new btDefaultMotionState(groundTransform);
            btRigidBody::btRigidBodyConstructionInfo rbInfo(0.0f, myMotionState, groundShape, btVector3(0.0f, 0.0f, 0.0f));
            m_groundBody = new btRigidBody(rbInfo);

            m_dynamicsWorld->addRigidBody(m_groundBody);
            std::cout << "[Physics] Infinite ground plane enabled." << std::endl;
        }
    }
    else
    {
        if (m_groundBody)
        {
            m_dynamicsWorld->removeRigidBody(m_groundBody);
            if (m_groundBody->getMotionState())
            {
                delete m_groundBody->getMotionState();
            }
            btCollisionShape *shape = m_groundBody->getCollisionShape();
            if (shape)
            {
                m_collisionShapes.erase(std::remove(m_collisionShapes.begin(), m_collisionShapes.end(), shape), m_collisionShapes.end());
                delete shape;
            }
            delete m_groundBody;
            m_groundBody = nullptr;
            std::cout << "[Physics] Infinite ground plane disabled." << std::endl;
        }
    }
}