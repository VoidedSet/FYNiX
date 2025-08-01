#include "Animator.h"

Animator::Animator(Animation *animation)
{
    m_currentTime = 0.0f;
    m_currentAnimation = animation;

    m_FinalBoneMatrices.reserve(250);

    for (int i = 0; i < 250; i++)

        m_FinalBoneMatrices.push_back(glm::mat4(1.f));
}

void Animator::Update(float dt)
{
    m_DeltaTime = dt;
    if (m_currentAnimation)
    {
        m_currentTime += m_currentAnimation->GetTicksPerSecond() * dt;
        m_currentTime = fmod(m_currentTime, m_currentAnimation->GetDuration());
        calculateBoneTransform(&m_currentAnimation->GetRootNode(), glm::mat4(1.f));
    }
}

void Animator::Play(Animation *pAnimation)
{
    m_currentAnimation = pAnimation;
    m_currentTime = 0.f;
}

void Animator::calculateBoneTransform(const AssimpNodeData *node, glm::mat4 parentTransform)
{
    std::string nodeName = node->name;
    glm::mat4 nodeTransform = node->transformation;

    Bone *Bone = m_currentAnimation->FindBone(nodeName);

    if (Bone)
    {
        Bone->Update(m_currentTime);
        nodeTransform = Bone->GetLocalTransform();
    }

    glm::mat4 globalTransformation = parentTransform * nodeTransform;

    auto boneInfoMap = m_currentAnimation->GetBoneIDMap();
    if (boneInfoMap.find(nodeName) != boneInfoMap.end())
    {
        int index = boneInfoMap[nodeName].id;
        glm::mat4 offset = boneInfoMap[nodeName].offset;

        if (index >= m_FinalBoneMatrices.size())
        {
            std::cerr << "[Animator] Resizing finalBoneMatrices: index " << index << " out of bounds!" << std::endl;
            m_FinalBoneMatrices.resize(index + 1, glm::mat4(1.0f));
        }

        m_FinalBoneMatrices[index] = globalTransformation * offset;
    }

    for (int i = 0; i < node->childrenCount; i++)
        calculateBoneTransform(&node->children[i], globalTransformation);
}