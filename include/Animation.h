#pragma once

#include <string>
#include <vector>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

struct MorphFrame
{
    float time;
    std::vector<float> weights;
};

class Animation
{
public:
    std::string name, targetMeshName;
    std::vector<std::vector<glm::vec3>> morphDeltas; // 2d vector. i think this shud be storing list of delta vec3 per frame?
    std::vector<MorphFrame> keyFrames;

    Animation(const std::string &name) : name(name) {}
    Animation() = default;

    size_t getMorphTargetCount() const { return morphDeltas.size(); }
    size_t getFrameCount() const { return keyFrames.size(); }
};