#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <assimp/scene.h>

// Forward-declare structs from Model.h to avoid circular dependency
struct Bone;
struct Skeleton;

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
    std::string name;
    float duration = 0.0f;
    float ticksPerSecond = 1.0f;
    std::unordered_map<std::string, BoneTransformTrack> boneTransforms = {};
};

class Animator
{
public:
    // Public state for easy UI binding
    bool isPaused = false;
    float currentTime = 0.f;
    int currentAnimationIndex = -1;
    std::vector<std::string> animationNames;

    Animator();

    void loadAnimation(const aiScene *scene);
    void updateAnimation(float deltaTime, Skeleton &skeleton, std::vector<glm::mat4> &finalBoneMatrices, const glm::mat4 &globalInverseTransform);
    void updatePose(Skeleton &skeleton, std::vector<glm::mat4> &finalBoneMatrices, const glm::mat4 &globalInverseTransform);

    // --- Playback Controls ---
    void play();
    void pause();
    void setAnimation(int index);
    void seek(float time, Skeleton &skeleton, std::vector<glm::mat4> &finalBoneMatrices, const glm::mat4 &globalInverseTransform);

    // --- Data Access ---
    Animation *getCurrentAnimation();

private:
    std::vector<Animation> animations;

    void getPose(Animation &animation, Bone &skeletonBone, float dt, std::vector<glm::mat4> &output, const glm::mat4 &parentTransform, const glm::mat4 &globalInverseTransform);
    std::pair<unsigned int, float> getTimeFraction(std::vector<float> &times, float &dt);

    // Helper converters
    glm::vec3 assimpToGlmVec3(const aiVector3D &vec);
    glm::quat assimpToGlmQuat(const aiQuaternion &quat);
};