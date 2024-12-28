#version 430 core

////////////////////////////////////////////////////////////////////////////////
// Uniforms & Inputs
////////////////////////////////////////////////////////////////////////////////
uniform bool isParticlePass; // again, same uniform used as in .vert
uniform sampler2D uTex;      // Could be used for both geometry or particle

// For the object pass:
layout(location = 2) uniform vec3 uDirLightDir;     
layout(location = 3) uniform vec3 uDirLightDiffuse; 
layout(location = 4) uniform vec3 uSceneAmbient;    
layout(location = 5) uniform bool uUseTexture;

// Point Light block (same as your original)
struct PointLight {
    vec3 position;
    float padding1;
    vec3 color;
    float padding2;
    vec3 normal;
    float radius;
};

layout(std140, binding=1) uniform PointLightBlock {
    PointLight lights[3];
};

// Particle pass input
in vec2 v2f_PointUV;  // not strictly used, but can be if you want custom logic

// Object pass inputs
in vec3 v2fColor;
in vec3 v2fNormal;
in vec2 v2fTexCoord;
in vec3 v2fKa;
in vec3 v2fKd;
in vec3 v2fKs;
in float v2fNs;
in vec3 v2fKe;
in vec3 v2fPosition;

out vec4 outColor;

////////////////////////////////////////////////////////////////////////////////
// Functions (object pass)
////////////////////////////////////////////////////////////////////////////////
float dotProduct(vec3 a, vec3 b) {
    return a.x*b.x + a.y*b.y + a.z*b.z;
}

vec3 calculatePointLight(PointLight light, vec3 normal, vec3 viewDir, vec3 position, vec3 kd, vec3 ks, float shininess)
{
    vec3 lightDir = light.position - position;
    float dist2 = dotProduct(lightDir, lightDir);
    float dist  = sqrt(dist2);
    lightDir    = lightDir / dist; // normalize

    float attenuation = 1.0 / (1.0 + (dist2)/(light.radius*light.radius));

    float diff = max(dotProduct(normal, lightDir),0.0);
    vec3 diffuse = kd * light.color * diff * attenuation;

    vec3 halfway=normalize(lightDir + viewDir);
    float spec = pow(max(dotProduct(normal, halfway), 0.0), shininess);
    vec3 specular = ks * light.color * spec * attenuation;
    return diffuse + specular;
}

////////////////////////////////////////////////////////////////////////////////
// main()
////////////////////////////////////////////////////////////////////////////////
void main()
{
    if(isParticlePass)
    {
        // *** PARTICLE FRAGMENT PASS ***
        // We do a point sprite read from uTex using gl_PointCoord
        // plus additive blending
        vec2 uv = gl_PointCoord;  // built-in for point sprites
        vec4 texColor = texture(uTex, uv);

        outColor = texColor;  // alpha-based shape
    }
    else
    {
        // *** NORMAL OBJECT PASS ***
        vec3 normal = normalize(v2fNormal);
        vec3 baseColor = (uUseTexture)
            ? texture(uTex, v2fTexCoord).rgb
            : v2fColor;

        // view direction
        vec3 viewDir = normalize(-v2fPosition);

        // Ambient
        vec3 ambient = v2fKa * uSceneAmbient;

        // Dir light
        float nDotL = max(dotProduct(normal, uDirLightDir), 0.0);
        vec3 dirDiffuse = v2fKd * uDirLightDiffuse * nDotL;

        vec3 halfDir = normalize(uDirLightDir + viewDir);
        float specFactor = pow(max(dotProduct(normal, halfDir),0.0), v2fNs);
        vec3 dirSpecular = v2fKs * uDirLightDiffuse * specFactor;

        // Summation from point lights
        vec3 pointSum = vec3(0.0);
        for(int i=0; i<3; i++){
            pointSum += calculatePointLight(lights[i], normal, viewDir, v2fPosition,
                                            v2fKd, v2fKs, v2fNs);
        }

        vec3 emission = v2fKe;
        vec3 lighting = ambient + dirDiffuse + dirSpecular + pointSum + emission;

        outColor = vec4(lighting * baseColor, 1.0);
    }
}
