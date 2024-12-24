#version 430

// Input data
layout( location = 0 ) in vec3 iPosition;  // Vertex position (x, y, z)
layout( location = 1 ) in vec3 iColor;     // Vertex color
layout( location = 2 ) in vec3 iNormal;    // Vertex normal
layout( location = 3 ) in vec2 iTexCoord;  // Vertex texture coordinates
layout( location = 4 ) in vec3 iKa;
layout( location = 5 ) in vec3 iKd;
layout( location = 6 ) in vec3 iKs;
layout( location = 7 ) in float iNs;
layout( location = 8 ) in vec3 iKe;

// Uniforms
layout( location = 0 ) uniform mat4 uProjCameraWorld; // Projection and camera matrix
layout( location = 1 ) uniform mat3 uNormalMatrix;    // Normal matrix for transforming normals
layout( location = 6 ) uniform vec2 uMin;            // Min X and Z
layout( location = 7 ) uniform vec2 uDiff;           // Diff X and Z

// Output attributes to the fragment shader
out vec3 v2fColor;    // Interpolated color
out vec3 v2fNormal;   // Interpolated normal
out vec2 v2fTexCoord; // Interpolated texture coordinates
out vec3 v2fKa;       // Ambience reflectivity (passed to fragment shader)
out vec3 v2fKd;       // Diffuse reflectivity (passed to fragment shader)
out vec3 v2fKs;       // Specular reflectivity (passed to fragment shader)
out float v2fNs;      // Shininess (passed to fragment shader)
out vec3 v2fKe;       // Emission (passed to fragment shader)

void main()
{
    // Copy input color to the output color attribute
    v2fColor = iColor;

    // Apply normal matrix to the normal and pass it as output
    v2fNormal = normalize(uNormalMatrix * iNormal);

    // Normalize the x and z positions to [0, 1] based on terrain dimensions
    float u = (iPosition.x - uMin.x) / uDiff.x;
    float v = (iPosition.z - uMin.y) / uDiff.y;

    // Assign the normalized values to texture coordinates
    v2fTexCoord = vec2(u, v);

    // Pass the material properties to the fragment shader
    v2fKa = iKa;
    v2fKd = iKd;
    v2fKs = iKs;
    v2fNs = iNs;
    v2fKe = iKe;

    // Apply the projection-camera-world transformation to the vertex position
    gl_Position = uProjCameraWorld * vec4(iPosition, 1.0);
}