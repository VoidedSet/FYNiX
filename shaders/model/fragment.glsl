#version 330 core
out vec4 FragColor;

uniform vec3 uCamPos;

uniform int numLights;  // pass number of active lights
uniform vec3 lightPositions[16];  // max 16 lights
uniform vec3 lightColors[16];

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};
uniform Material material;
uniform bool useSpecularMap;

uniform sampler2D texture_diffuse0;
uniform sampler2D texture_specular0;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

void main() {
    vec3 result = vec3(0.0);
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(uCamPos - FragPos);

    for (int i = 0; i < numLights; ++i) {
        // Ambient
        vec3 ambient = material.ambient * lightColors[i];

        // Diffuse
        vec3 lightDir = normalize(lightPositions[i] - FragPos);
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = material.diffuse * diff * lightColors[i];

        // Specular
        vec3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
        vec3 specularTex = useSpecularMap ? texture(texture_specular0, TexCoord).rgb : vec3(1.0);
        vec3 specular = material.specular * spec * lightColors[i] * specularTex;

        result += (ambient + diffuse + specular);
    }

    vec4 texColor = texture(texture_diffuse0, TexCoord);
    FragColor = vec4(result * texColor.rgb, texColor.a);
}
