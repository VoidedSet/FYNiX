#version 330 core
layout (location = 0) in vec4 aPosTex;    // Vertex position and texture coordinate
layout (location = 1) in vec4 aInstancePosSize; // Per-instance position and size
layout (location = 2) in vec4 aInstanceColor; // Per-instance color

out vec4 vColor; // Pass the color to the fragment shader

uniform mat4 view;
uniform mat4 projection;

void main()
{
    // The position is defined by the instance position and the vertex position.
    // The vertex position is offset from the instance position in view space,
    // so the particles always face the camera (billboard effect).
    vec3 positionInViewSpace = (view * vec4(aInstancePosSize.xyz, 1.0)).xyz;
    vec3 billboardPosition = positionInViewSpace + vec3(aPosTex.x, aPosTex.y, 0.0) * aInstancePosSize.w;

    gl_Position = projection * vec4(billboardPosition, 1.0);

    // Pass the color to the fragment shader.
    vColor = aInstanceColor;
}
