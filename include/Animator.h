#pragma once

#include <iostream>
#include <vector>

#include "Animation.h"

class Animator
{
public:
    void Play(Animation *anim);
    void Update(float deltaTime);
    std::vector<float> getCurrentWeights() const;

    float getTime() const { return currentTime; }
    Animation *getCurrentAnimation() const { return currentAnimation; }
    float getBlendFactor() const { return blendFactor; } // ← ADD THIS LINE

private:
    Animation *currentAnimation = nullptr;
    float currentTime = 0.0f;
    float blendFactor = 0.f;
    std::vector<float> prevWeightsVec;
    std::vector<float> nextWeightsVec;
};
