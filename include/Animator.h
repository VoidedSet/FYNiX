#pragma once

#include <vector>
#include <string>
#include <unordered_map>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <assimp/scene.h>

struct BoneTransformTrack
{
    std::vector<float> positionTimestamps = {};
    std::vector<float> rotationTimestamps = {};
    std::vector<float> scaleTimestamps = {};

    std::vector<glm::vec3> positions = {};
    std::vector<glm::quat> rotations = {};
    std::vector<glm::vec3> scales = {};
};

struct Animation
{
    float duration = 0.0f;
    float ticksPerSecond = 1.0f;
    std::unordered_map<std::string, BoneTransformTrack> boneTransforms = {};
};

class Animator
{
public:
    Animator();
    void loadAnimation(const aiScene *scene);
    void updateAnimation(float deltaTime, struct Skeleton &skeleton, std::vector<glm::mat4> &finalBoneMatrices, const glm::mat4 &globalInverseTransform);

private:
    std::vector<Animation> animations;
    float currentTime = 0.f;

    void getPose(Animation &animation, struct Bone &skeleton, float dt, std::vector<glm::mat4> &output, const glm::mat4 &parentTransform, const glm::mat4 &globalInverseTransform);
    std::pair<unsigned int, float> getTimeFraction(std::vector<float> &times, float &dt);

    glm::vec3 assimpToGlmVec3(aiVector3D vec);
    glm::quat assimpToGlmQuat(aiQuaternion quat);
};