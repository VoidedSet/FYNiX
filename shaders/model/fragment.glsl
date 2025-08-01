#version 330 core
out vec4 FragColor;

uniform vec3 uCamPos;

uniform int numLights;  // pass number of active lights
uniform vec3 lightPositions[16];  // max 16 lights pls
uniform vec3 lightColors[16];

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
        float ambientStrength = 0.1;
        vec3 ambient = ambientStrength * lightColors[i];

        vec3 lightDir = normalize(lightPositions[i] - FragPos);
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = diff * lightColors[i];

        float specularStrength = 0.5;
        vec3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
        vec3 specular = specularStrength * spec * lightColors[i];

        result += (ambient + diffuse + specular);
    }

    vec4 texColor = texture(texture_diffuse0, TexCoord);
    FragColor = vec4(texColor.rgb, texColor.a);
}
