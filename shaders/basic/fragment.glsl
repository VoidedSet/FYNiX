#version 330 core
out vec4 FragColor;

uniform vec3 uColor;
uniform sampler2D texture_diffuse0;
uniform sampler2D texture_specular0;

// in vec4 vertexColor;
in vec2 TexCoord;

void main(){
    //FragColor = vec4(vertexColor.x, uColor.y, vertexColor.zw);
    // FragColor = mix(texture(texture_diffuse0, TexCoord), texture(texture_specular0, TexCoord * -1.0), uColor.y);
    FragColor  = texture(texture_diffuse0, TexCoord);
}
