#version 330 core
out vec4 FragColor;

uniform vec3 uCamPos;

uniform int numLights;  // pass number of active lights
uniform vec3 lightPositions[16];  // max 16 lights
uniform vec3 lightColors[16];
uniform int lightTypes[16];
uniform vec3 lightDirections[16];

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
        vec3 lightDir;
        float attenuation = 1.0;
        
        if (lightTypes[i] == 0) { // DIRECTIONAL
            lightDir = normalize(-lightDirections[i]);
        } else { // POINT or SPOT
            lightDir = normalize(lightPositions[i] - FragPos);
            float distance = length(lightPositions[i] - FragPos);
            // Standard attenuation
            attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);
        }

        // Ambient
        vec3 ambient = material.ambient * lightColors[i] * attenuation;

        // Diffuse
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = material.diffuse * diff * lightColors[i] * attenuation;

        // Specular
        vec3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
        vec3 specularTex = useSpecularMap ? texture(texture_specular0, TexCoord).rgb : vec3(1.0);
        vec3 specular = material.specular * spec * lightColors[i] * specularTex * attenuation;

        if (lightTypes[i] == 2) { // SPOT LIGHT
            vec3 spotDir = normalize(lightDirections[i]);
            float theta = dot(-lightDir, spotDir);
            float epsilon = 0.976 - 0.966; // cos(12.5) - cos(15)
            float intensity = clamp((theta - 0.966) / epsilon, 0.0, 1.0);
            diffuse *= intensity;
            specular *= intensity;
        }

        result += (ambient + diffuse + specular);
    }

    vec4 texColor = texture(texture_diffuse0, TexCoord);
    FragColor = vec4(result * texColor.rgb, texColor.a);
}
