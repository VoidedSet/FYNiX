#version 330 core
out vec4 FragColor;

uniform vec3 uColor;
uniform sampler2D texture1;
uniform sampler2D texture2;

// in vec4 vertexColor;
in vec2 TexCoord;

void main(){
    //FragColor = vec4(vertexColor.x, uColor.y, vertexColor.zw);
    FragColor = mix(texture(texture1, TexCoord), texture(texture2, TexCoord * -1.0), uColor.y);
}
