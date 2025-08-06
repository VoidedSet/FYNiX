#include "Animator.h"
#include "Model.h" // For Bone and Skeleton structs
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

Animator::Animator() {}

void Animator::loadAnimation(const aiScene *scene)
{
    if (!scene || scene->mNumAnimations < 1)
    {
        return;
    }

    animations.clear();
    animationNames.clear();

    for (unsigned int i = 0; i < scene->mNumAnimations; ++i)
    {
        aiAnimation *aiAnim = scene->mAnimations[i];
        Animation anim;

        anim.name = aiAnim->mName.C_Str();
        animationNames.push_back(anim.name);

        anim.ticksPerSecond = (aiAnim->mTicksPerSecond != 0.0f) ? aiAnim->mTicksPerSecond : 25.0f;
        anim.duration = aiAnim->mDuration;

        for (unsigned int j = 0; j < aiAnim->mNumChannels; ++j)
        {
            aiNodeAnim *channel = aiAnim->mChannels[j];
            BoneTransformTrack track;
            for (unsigned int k = 0; k < channel->mNumPositionKeys; ++k)
            {
                track.positionTimestamps.push_back(channel->mPositionKeys[k].mTime);
                track.positions.push_back(assimpToGlmVec3(channel->mPositionKeys[k].mValue));
            }
            for (unsigned int k = 0; k < channel->mNumRotationKeys; ++k)
            {
                track.rotationTimestamps.push_back(channel->mRotationKeys[k].mTime);
                track.rotations.push_back(assimpToGlmQuat(channel->mRotationKeys[k].mValue));
            }
            for (unsigned int k = 0; k < channel->mNumScalingKeys; ++k)
            {
                track.scaleTimestamps.push_back(channel->mScalingKeys[k].mTime);
                track.scales.push_back(assimpToGlmVec3(channel->mScalingKeys[k].mValue));
            }
            anim.boneTransforms[channel->mNodeName.C_Str()] = track;
        }
        animations.push_back(anim);
    }

    if (!animations.empty())
    {
        currentAnimationIndex = 0; // Default to the first animation
    }
}

void Animator::updateAnimation(float deltaTime, Skeleton &skeleton, std::vector<glm::mat4> &finalBoneMatrices, const glm::mat4 &globalInverseTransform)
{
    if (isPaused || currentAnimationIndex < 0 || animations.empty())
    {
        return;
    }

    Animation &anim = animations[currentAnimationIndex];
    currentTime += deltaTime * anim.ticksPerSecond;

    if (currentTime > anim.duration)
    {
        currentTime = fmod(currentTime, anim.duration);
    }

    updatePose(skeleton, finalBoneMatrices, globalInverseTransform);
}

void Animator::updatePose(Skeleton &skeleton, std::vector<glm::mat4> &finalBoneMatrices, const glm::mat4 &globalInverseTransform)
{
    if (currentAnimationIndex < 0 || animations.empty())
        return;

    if (finalBoneMatrices.size() != skeleton.boneCount)
    {
        finalBoneMatrices.resize(skeleton.boneCount, glm::mat4(1.0f));
    }

    std::vector<glm::mat4> boneMatrices(skeleton.boneCount);
    getPose(animations[currentAnimationIndex], skeleton.rootBone, currentTime, boneMatrices, glm::mat4(1.0f), globalInverseTransform);

    for (size_t i = 0; i < boneMatrices.size(); ++i)
    {
        finalBoneMatrices[i] = boneMatrices[i];
    }
}

void Animator::play() { isPaused = false; }
void Animator::pause() { isPaused = true; }

void Animator::setAnimation(int index)
{
    if (index >= 0 && index < animations.size())
    {
        currentAnimationIndex = index;
        currentTime = 0.0f; // Reset time when changing animation
    }
}

void Animator::seek(float time, Skeleton &skeleton, std::vector<glm::mat4> &finalBoneMatrices, const glm::mat4 &globalInverseTransform)
{
    if (currentAnimationIndex < 0)
        return;
    currentTime = glm::clamp(time, 0.0f, animations[currentAnimationIndex].duration);
    updatePose(skeleton, finalBoneMatrices, globalInverseTransform);
}

Animation *Animator::getCurrentAnimation()
{
    if (currentAnimationIndex < 0 || animations.empty())
        return nullptr;
    return &animations[currentAnimationIndex];
}

// The rest of the file (getPose, getTimeFraction, etc.) remains the same.
// Make sure these helpers are also in the file.
void Animator::getPose(Animation &animation, Bone &skeletonBone, float dt, std::vector<glm::mat4> &output, const glm::mat4 &parentTransform, const glm::mat4 &globalInverseTransform)
{
    auto it = animation.boneTransforms.find(skeletonBone.name);
    if (it == animation.boneTransforms.end())
    {
        glm::mat4 globalTransform = parentTransform;
        output[skeletonBone.id] = globalInverseTransform * globalTransform * skeletonBone.offset;
        for (Bone &child : skeletonBone.children)
        {
            getPose(animation, child, dt, output, globalTransform, globalInverseTransform);
        }
        return;
    }

    BoneTransformTrack &btt = it->second;

    glm::vec3 position;
    if (btt.positions.size() > 1)
    {
        std::pair<unsigned int, float> fp = getTimeFraction(btt.positionTimestamps, dt);
        position = glm::mix(btt.positions[fp.first - 1], btt.positions[fp.first], fp.second);
    }
    else
    {
        position = btt.positions[0];
    }

    glm::quat rotation;
    if (btt.rotations.size() > 1)
    {
        std::pair<unsigned int, float> fp = getTimeFraction(btt.rotationTimestamps, dt);
        rotation = glm::slerp(btt.rotations[fp.first - 1], btt.rotations[fp.first], fp.second);
    }
    else
    {
        rotation = btt.rotations[0];
    }

    glm::vec3 scale;
    if (btt.scales.size() > 1)
    {
        std::pair<unsigned int, float> fp = getTimeFraction(btt.scaleTimestamps, dt);
        scale = glm::mix(btt.scales[fp.first - 1], btt.scales[fp.first], fp.second);
    }
    else
    {
        scale = btt.scales[0];
    }

    glm::mat4 localTransform = glm::translate(glm::mat4(1.0f), position) * glm::toMat4(rotation) * glm::scale(glm::mat4(1.0f), scale);
    glm::mat4 globalTransform = parentTransform * localTransform;
    output[skeletonBone.id] = globalInverseTransform * globalTransform * skeletonBone.offset;
    for (Bone &child : skeletonBone.children)
    {
        getPose(animation, child, dt, output, globalTransform, globalInverseTransform);
    }
}

std::pair<unsigned int, float> Animator::getTimeFraction(std::vector<float> &times, float &dt)
{
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
    float frac = (end - start > 0.0f) ? (dt - start) / (end - start) : 0.0f;
    return {segment, frac};
}

glm::vec3 Animator::assimpToGlmVec3(const aiVector3D &vec) { return glm::vec3(vec.x, vec.y, vec.z); }
glm::quat Animator::assimpToGlmQuat(const aiQuaternion &quat) { return glm::quat(quat.w, quat.x, quat.y, quat.z); }