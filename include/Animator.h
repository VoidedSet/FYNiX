#pragma once

#include <vector>
#include <map>
#include <string>
#include <iostream>

#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/mesh.h>
#include <assimp/material.h>
#include <assimp/MathFunctions.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include "Model.h"
#include "Animation.h"

class Animator
{
public:
    Animator(Animation *animation);

    void Update(float dt);
    void Play(Animation *pAnimation);

    void calculateBoneTransform(const AssimpNodeData *node, glm::mat4 parentTransform);

    std::vector<glm::mat4> GetFinalBoneMatrices()
    {
        return m_FinalBoneMatrices;
    }

private:
    std::vector<glm::mat4> m_FinalBoneMatrices;
    Animation *m_currentAnimation;
    float m_currentTime;
    float m_DeltaTime;
};