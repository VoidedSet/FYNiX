#version 330 core
in vec4 vColor; // Interpolated color from the vertex shader
out vec4 FragColor;

void main()
{
    // Output the interpolated color directly.
    FragColor = vColor;
}
