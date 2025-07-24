uniform sampler2D texture_diffuse;
uniform sampler2D texture_specular;

in vec2 TexCoord;
out vec4 FragColor;

void main() {
    FragColor = texture(texture_diffuse, TexCoord);
}
