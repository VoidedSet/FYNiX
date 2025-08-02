#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

layout(location = 3) in vec3 aMorphPos1;
layout(location = 4) in vec3 aMorphPos2;

uniform float blendFactor;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;

void main() {
    vec3 morphedPos = mix(aMorphPos1, aMorphPos2, blendFactor);
    vec3 finalPos = aPos + morphedPos;

    gl_Position = projection * view * model * vec4(finalPos, 1.0);
    FragPos = vec3(model * vec4(finalPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoord = aTexCoord;
}
