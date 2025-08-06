#include "Animator.h"
#include "Model.h" // For the Bone and Skeleton structs, consider moving them to a separate file if you want to completely decouple.
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

Animator::Animator() {}

void Animator::loadAnimation(const aiScene *scene)
{
    if (!scene || scene->mNumAnimations < 1)
    {
        return;
    }

    // loading the first Animation
    aiAnimation *anim = scene->mAnimations[0];
    Animation animation;

    if (anim->mTicksPerSecond != 0.0f)
        animation.ticksPerSecond = anim->mTicksPerSecond;
    else
        animation.ticksPerSecond = 1;

    animation.duration = anim->mDuration;
    animation.boneTransforms = {};

    // load positions, rotations, and scales for each bone
    // each channel represents each bone
    for (unsigned int i = 0; i < anim->mNumChannels; i++)
    {
        aiNodeAnim *channel = anim->mChannels[i];
        BoneTransformTrack track;
        for (unsigned int j = 0; j < channel->mNumPositionKeys; j++)
        {
            track.positionTimestamps.push_back(channel->mPositionKeys[j].mTime);
            track.positions.push_back(assimpToGlmVec3(channel->mPositionKeys[j].mValue));
        }
        for (unsigned int j = 0; j < channel->mNumRotationKeys; j++)
        {
            track.rotationTimestamps.push_back(channel->mRotationKeys[j].mTime);
            track.rotations.push_back(assimpToGlmQuat(channel->mRotationKeys[j].mValue));
        }
        for (unsigned int j = 0; j < channel->mNumScalingKeys; j++)
        {
            track.scaleTimestamps.push_back(channel->mScalingKeys[j].mTime);
            track.scales.push_back(assimpToGlmVec3(channel->mScalingKeys[j].mValue));
        }
        animation.boneTransforms[channel->mNodeName.C_Str()] = track;
    }
    animations.push_back(animation);
}

void Animator::updateAnimation(float deltaTime, struct Skeleton &skeleton, std::vector<glm::mat4> &finalBoneMatrices, const glm::mat4 &globalInverseTransform)
{
    if (animations.empty())
        return;

    currentTime += deltaTime * animations[0].ticksPerSecond;

    if (currentTime > animations[0].duration)
        currentTime = fmod(currentTime, animations[0].duration);

    if (finalBoneMatrices.size() != skeleton.boneCount)
        finalBoneMatrices.resize(skeleton.boneCount, glm::mat4(1.0f));

    std::vector<glm::mat4> boneMatrices(skeleton.boneCount);
    glm::mat4 identity = glm::mat4(1.0f);

    getPose(animations[0], skeleton.rootBone, currentTime, boneMatrices, identity, globalInverseTransform);

    for (size_t i = 0; i < boneMatrices.size(); i++)
        finalBoneMatrices[i] = boneMatrices[i];
}

// In Animator.cpp
void Animator::getPose(Animation &animation, struct Bone &skeletonBone, float dt, std::vector<glm::mat4> &output, const glm::mat4 &parentTransform, const glm::mat4 &globalInverseTransform)
{
    auto it = animation.boneTransforms.find(skeletonBone.name);
    if (it == animation.boneTransforms.end())
    {
        // No animation track for this bone, just propagate parent transform
        glm::mat4 globalTransform = parentTransform; // Or apply its bind pose transform if needed
        output[skeletonBone.id] = globalInverseTransform * globalTransform * skeletonBone.offset;

        for (Bone &child : skeletonBone.children)
        {
            getPose(animation, child, dt, output, globalTransform, globalInverseTransform);
        }
        return;
    }

    BoneTransformTrack &btt = it->second;

    // --- Interpolate Position ---
    glm::vec3 position;
    if (btt.positions.size() > 1)
    {
        std::pair<unsigned int, float> fp = getTimeFraction(btt.positionTimestamps, dt);
        glm::vec3 p1 = btt.positions[fp.first - 1];
        glm::vec3 p2 = btt.positions[fp.first];
        position = glm::mix(p1, p2, fp.second);
    }
    else
    {
        position = btt.positions[0];
    }

    // --- Interpolate Rotation ---
    glm::quat rotation;
    if (btt.rotations.size() > 1)
    {
        std::pair<unsigned int, float> fp = getTimeFraction(btt.rotationTimestamps, dt);
        glm::quat r1 = btt.rotations[fp.first - 1];
        glm::quat r2 = btt.rotations[fp.first];
        rotation = glm::slerp(r1, r2, fp.second);
    }
    else
    {
        rotation = btt.rotations[0];
    }

    // --- Interpolate Scale ---
    glm::vec3 scale;
    if (btt.scales.size() > 1)
    {
        std::pair<unsigned int, float> fp = getTimeFraction(btt.scaleTimestamps, dt);
        glm::vec3 s1 = btt.scales[fp.first - 1];
        glm::vec3 s2 = btt.scales[fp.first];
        scale = glm::mix(s1, s2, fp.second);
    }
    else
    {
        scale = btt.scales[0];
    }

    // Combine transforms
    glm::mat4 positionMat = glm::translate(glm::mat4(1.0f), position);
    glm::mat4 rotationMat = glm::toMat4(rotation);
    glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), scale);

    glm::mat4 localTransform = positionMat * rotationMat * scaleMat;
    glm::mat4 globalTransform = parentTransform * localTransform;

    output[skeletonBone.id] = globalInverseTransform * globalTransform * skeletonBone.offset;

    for (Bone &child : skeletonBone.children)
    {
        getPose(animation, child, dt, output, globalTransform, globalInverseTransform);
    }
}

// In Animator.cpp
std::pair<unsigned int, float> Animator::getTimeFraction(std::vector<float> &times, float &dt)
{
    // This logic now matches your original implementation which was correct.
    unsigned int segment = 1;
    while (segment < times.size() && dt > times[segment])
    {
        segment++;
    }

    if (segment >= times.size())
    {
        segment = times.size() - 1;
    }

    float start = times[segment - 1];
    float end = times[segment];
    float frac = (dt - start) / (end - start);

    return {segment, frac};
}

glm::vec3 Animator::assimpToGlmVec3(aiVector3D vec)
{
    return glm::vec3(vec.x, vec.y, vec.z);
}

glm::quat Animator::assimpToGlmQuat(aiQuaternion quat)
{
    glm::quat q;
    q.x = quat.x;
    q.y = quat.y;
    q.z = quat.z;
    q.w = quat.w;
    return q;
}