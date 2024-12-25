#version 430
// Input attributes
// These should match the outputs from the vertex shader.
in vec3 v2fColor;
in vec3 v2fNormal;
in vec2 v2fTexCoord;
out vec3 v2fKa;       // Ambience reflectivity (Ka)
in vec3 v2fKd;        // Diffuse reflectivity (Kd)
in vec3 v2fKs;        // Specular reflectivity (Ks)
in float v2fNs;       // Shininess (Ns)
in vec3 v2fKe;        // Emission (Ke)

// Uniform data
// For now, we use the “register style” uniforms. Similar to the vertex shader inputs, the
// layout( location = N )
// syntax allows us to specify uniform locations in the shader (which avoids the need to query them later,
// via glGetUniformLocation()). Again, see
// https://www.khronos.org/opengl/wiki/Layout Qualifier (GLSL)
// for more information

// Uniform data
layout(location = 2) uniform vec3 uLightDir;          // Light direction
layout(location = 3) uniform vec3 uLightDiffuse;      // Light diffuse color
layout(location = 4) uniform vec3 uSceneAmbient;      // Ambient light color
layout(binding = 0) uniform sampler2D uTexture;       // Texture sampler
layout(location = 5) uniform bool uUseTexture;        // Flag to use texture


// Fragment shader outputs
// For now, we have a single output, a RGB (=vec3) color. This is written to the color attachment with
// index 0 (the location value below). For the moment, we are drawing the default framebuffer, which has
// only one color attachment: color attachment 0, which is the window’s back buffer.
layout( location = 0 ) out vec3 oColor;


void main()
{
	// Normalize the interpolated normal
    vec3 normal = normalize(v2fNormal);
    
    // Calculate lighting factor
    float nDotL = max(0.0, dot(normal, uLightDir));
    
    // Get the base color either from texture or vertex color
    vec3 baseColor;
    if (uUseTexture) {
        baseColor = texture(uTexture, v2fTexCoord).rgb;
    } else {
        baseColor = v2fColor;
    }
    
    // Apply lighting to the base color
    oColor = (uSceneAmbient + nDotL * uLightDiffuse) * baseColor;

	// oColor = texture( uTexture, v2fTexCoord ).rgb;		// For texturing only


	 //oColor = normal;		// Use this as a debug method to visualise normals as colours 

	// oColor = vec3(v2fTexCoord[0], v2fTexCoord[1], 0.0);		// Use this as a debug to visualise texture co-ords as colours 
	
	// oColor = v2fColor;

    
    
    
    /*  UNTESTED BLINN-PHONG LIGHTING MODEL. */

    // Normalize the interpolated normal
    // vec3 normal = normalize(v2fNormal);
    
    // Calculate the diffuse lighting factor (Lambertian reflection)
    //float nDotL = max(0.0, dot(normal, uLightDir));

    // Get the base color either from texture or vertex color
    //vec3 baseColor;
    //if (uUseTexture) {
    //    baseColor = texture(uTexture, v2fTexCoord).rgb;
    //} else {
    //    baseColor = v2fColor;
    //}

    // Calculate diffuse lighting contribution
    //vec3 diffuse = nDotL * uLightDiffuse * v2fKd;

    // Calculate specular lighting (Phong reflection model)
    //vec3 viewDir = normalize(-gl_FragCoord.xyz);  // Simplified view direction (camera looking from the origin)
    //vec3 reflectDir = reflect(-uLightDir, normal);
    //float spec = pow(max(dot(viewDir, reflectDir), 0.0), v2fNs);
    //vec3 specular = spec * uLightDiffuse * v2fKs;

    // Combine diffuse and specular lighting
    //vec3 lighting = diffuse + specular;

    // Apply ambient lighting
    //vec3 ambient = uSceneAmbient * v2fKd;

    // Add emission to the final color (Ke)
    //vec3 emission = v2fKe;

    // Combine the lighting components with the base color
    //oColor = ambient + lighting + emission;

    // Optionally, multiply by the base color to apply texture and color
    //oColor *= baseColor;
}

