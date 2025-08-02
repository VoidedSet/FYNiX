#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in ivec4 boneIds;
layout (location = 4) in vec4 boneWeights;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform mat4 bone_transforms[200];
uniform bool isAnimated;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;

void main(){

mat4 skinningTransform = mat4(1.0);

if (isAnimated) {
    mat4 boneTransform = mat4(0.0);
    boneTransform += bone_transforms[int(boneIds.x)] * boneWeights.x;
    boneTransform += bone_transforms[int(boneIds.y)] * boneWeights.y;
    boneTransform += bone_transforms[int(boneIds.z)] * boneWeights.z;
    boneTransform += bone_transforms[int(boneIds.w)] * boneWeights.w;
    skinningTransform = boneTransform;
}

vec4 skinnedPos = skinningTransform * vec4(aPos, 1.0);

gl_Position = projection * view * model * skinnedPos;
FragPos = vec3(model * skinnedPos);
Normal = mat3(transpose(inverse(model))) * mat3(skinningTransform) * aNormal;
TexCoord = aTexCoord;
}