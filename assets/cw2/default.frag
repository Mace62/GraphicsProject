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

// Uniforms
layout(location = 2) uniform vec3 uLightDir;          // Light direction (world space, normalised)
layout(location = 3) uniform vec3 uLightDiffuse;      // Light diffuse color/intensity
layout(location = 4) uniform vec3 uSceneAmbient;      // Ambient light color/intensity
layout(binding = 0) uniform sampler2D uTexture;       // Texture sampler
layout(location = 5) uniform bool uUseTexture;        // Flag to use texture

// Outputs
layout(location = 0) out vec3 oColor;                 // Final fragment color

void main() {
    // Normalise the interpolated normal
    vec3 normal = normalize(v2fNormal);

    // Base color from texture or vertex color
    vec3 baseColor = uUseTexture ? texture(uTexture, v2fTexCoord).rgb : v2fColor;

    // Ambient lighting
    vec3 ambient = v2fKa * uSceneAmbient;

    // Diffuse lighting (Lambertian reflection)
    float nDotL = max(dot(normal, uLightDir), 0.0);   // Clamp to [0,1]
    vec3 diffuse = v2fKd * uLightDiffuse * nDotL;

    // Specular lighting (Blinn-Phong reflection model)
    vec3 viewDir = normalize(-gl_FragCoord.xyz);      // View direction (simplified to camera at origin)
    vec3 halfwayDir = normalize(uLightDir + viewDir); // Halfway vector
    float spec = pow(max(dot(normal, halfwayDir), 0.0), v2fNs); // Specular intensity
    vec3 specular = v2fKs * uLightDiffuse * spec;

    // Emission
    vec3 emission = v2fKe;

    // Combine components
    vec3 lighting = ambient + diffuse + specular + emission;

    // Final color (apply base color)
    oColor = lighting * baseColor;

    // Debugging options (uncomment one if needed)
    // oColor = normal;                                                 // Visualize normals
    // oColor = vec3(v2fTexCoord[0], v2fTexCoord[1], 0.0);              // Visualize texture coordinates
    // oColor = v2fColor;                                               // Use vertex color only
}
