// Animator.cpp
#include "Animator.h"

#include <algorithm> // for std::clamp
#include <cmath>

void Animator::Play(Animation *anim)
{
    currentAnimation = anim;
    currentTime = 0.0f;
}

void Animator::Update(float deltaTime)
{
    if (!currentAnimation || currentAnimation->keyFrames.empty())
        return;

    currentTime += deltaTime;

    // Wrap around (loop animation)
    float maxTime = currentAnimation->keyFrames.back().time;
    if (currentTime > maxTime)
        currentTime = fmod(currentTime, maxTime);

    // Find the 2 keyframes we're between
    int frameCount = currentAnimation->keyFrames.size();
    for (int i = 0; i < frameCount - 1; ++i)
    {
        const MorphFrame &kf1 = currentAnimation->keyFrames[i];
        const MorphFrame &kf2 = currentAnimation->keyFrames[i + 1];

        if (currentTime >= kf1.time && currentTime <= kf2.time)
        {
            float duration = kf2.time - kf1.time;
            blendFactor = (duration > 0.0f) ? (currentTime - kf1.time) / duration : 0.0f;
            std::cout << "[Blend Factor] " << blendFactor << std::endl;
            prevWeightsVec = kf1.weights;
            nextWeightsVec = kf2.weights;
            return;
        }
    }

    // If time is before first frame or after last
    prevWeightsVec = currentAnimation->keyFrames.front().weights;
    nextWeightsVec = currentAnimation->keyFrames.front().weights;
    blendFactor = 0.0f;
}

std::vector<float> Animator::getCurrentWeights() const
{
    std::vector<float> blended;
    blended.resize(prevWeightsVec.size());

    for (size_t i = 0; i < blended.size(); ++i)
    {
        blended[i] = (1.0f - blendFactor) * prevWeightsVec[i] + blendFactor * nextWeightsVec[i];
    }

    return blended;
}
