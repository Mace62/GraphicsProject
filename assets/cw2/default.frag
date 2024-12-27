#version 430

// Input attributes (from vertex shader)
in vec3 v2fColor;         // Vertex color
in vec3 v2fNormal;        // Vertex normal
in vec2 v2fTexCoord;      // Texture coordinates
in vec3 v2fKa;            // Ambient reflectivity (Ka)
in vec3 v2fKd;            // Diffuse reflectivity (Kd)
in vec3 v2fKs;            // Specular reflectivity (Ks)
in float v2fNs;           // Shininess (Ns)
in vec3 v2fKe;            // Emission (Ke)
in vec3 v2fPosition;      // Vertex position


// Uniforms
layout(location = 2) uniform vec3 uDirLightDir;          // Light direction (world space, normalised)
layout(location = 3) uniform vec3 uDirLightDiffuse;      // Light diffuse color/intensity
layout(location = 4) uniform vec3 uSceneAmbient;      // Ambient light color/intensity
layout(binding = 0) uniform sampler2D uTexture;       // Texture sampler
layout(location = 5) uniform bool uUseTexture;        // Flag to use texture

// Point Light structs and uniforms
struct PointLight {
    vec3 position;  // 12 bytes + 4 bytes padding
    vec3 color;     // 12 bytes + 4 bytes padding
    float radius;   // Aligned at 4 bytes
};

layout(std140, binding = 1) uniform PointLightBlock {
    PointLight lights[3];
};

out vec3 oColor;

float dotProduct(vec3 a, vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}


// Function to calculate point light contribution
vec3 calculatePointLight(PointLight light, vec3 normal, vec3 viewDir, vec3 position) {
    vec3 lightDir = light.position - position;
    float distance = length(lightDir);
    lightDir = normalize(lightDir);

    // Attenuation
    float attenuation = 1.0 / (1.0 + (distance * distance) / (light.radius * light.radius));

    // Diffuse
    float diff = max(dotProduct(normal, lightDir), 0.0);
    vec3 diffuse = v2fKd * light.color * diff * attenuation;

    // Specular
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dotProduct(normal, halfwayDir), 0.0), v2fNs);
    vec3 specular = v2fKs * light.color * spec * attenuation;

    //oColor = normalize(lightDir) * 0.5 + 0.5;  // Map direction to [0, 1]    // Visualise light dir
    // oColor = (light.position * 0.5) + 0.5; // Scale light position to [0, 1]
    return (diffuse + specular);
}

void main() {
    vec3 normal = normalize(v2fNormal);
    vec3 baseColor = uUseTexture ? texture(uTexture, v2fTexCoord).rgb : v2fColor;
    vec3 viewDir = normalize(-v2fPosition);

    // Ambient lighting
    vec3 ambient = v2fKa * uSceneAmbient;

    // Directional light calculation
    float nDotL = max(dotProduct(normal, uDirLightDir), 0.0);
    vec3 dirDiffuse = v2fKd * uDirLightDiffuse * nDotL;

    vec3 dirHalfwayDir = normalize(uDirLightDir + viewDir);
    float dirSpec = pow(max(dotProduct(normal, dirHalfwayDir), 0.0), v2fNs);
    vec3 dirSpecular = v2fKs * uDirLightDiffuse * dirSpec;

    // Accumulate point lights
    vec3 pointLighting = vec3(0.0);
    for(int i = 0; i < 3; i++) {
        pointLighting += calculatePointLight(lights[i], normal, viewDir, v2fPosition);
    }

    // Emission
    vec3 emission = v2fKe;

    // Combine all lighting components
    vec3 lighting = ambient + dirDiffuse + dirSpecular + pointLighting + emission;
    oColor = lighting * baseColor;
    // Debugging options (uncomment one if needed)
    // oColor = normal;                                                 // Visualize normals
    // oColor = vec3(v2fTexCoord[0], v2fTexCoord[1], 0.0);              // Visualize texture coordinates
    // oColor = v2fColor;                                               // Use vertex color only

}
