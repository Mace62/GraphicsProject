#version 430 core

////////////////////////////////////////////////////////////////////////////////
// Common Uniforms
////////////////////////////////////////////////////////////////////////////////
uniform bool isParticlePass;  // true => point sprites, false => normal objects

// For both geometry & particle pass:
uniform mat4 uProj;      // used by particle pass
uniform mat4 uView;      // used by particle pass
uniform sampler2D uTex;  // (some passes might not use it)
uniform float uPointSize = 32.0; // default point size for particles

////////////////////////////////////////////////////////////////////////////////
// Particle-specific Input (Used IF isParticlePass == true)
// location=0 in vec3 inPosition; // Will unify below
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Object-specific Input (Used IF isParticlePass == false)
////////////////////////////////////////////////////////////////////////////////
layout(location = 0) in vec3 iPosition;   // Vertex position (x, y, z)
layout(location = 1) in vec3 iColor;      // Vertex color
layout(location = 2) in vec3 iNormal;     // Vertex normal
layout(location = 3) in vec2 iTexCoord;   // Vertex texture coordinates
layout(location = 4) in vec3 iKa;
layout(location = 5) in vec3 iKd;
layout(location = 6) in vec3 iKs;
layout(location = 7) in float iNs;
layout(location = 8) in vec3 iKe;

// Additional Uniforms for object pass:
layout(location = 0) uniform mat4 uProjCameraWorld; // Projection * camera * world
layout(location = 1) uniform mat3 uNormalMatrix;    // Normal transform
layout(location = 6) uniform vec2 uMin;             // for texture coordinate
layout(location = 7) uniform vec2 uDiff;            // for texture coordinate

////////////////////////////////////////////////////////////////////////////////
// Outputs to fragment
////////////////////////////////////////////////////////////////////////////////

// For Particle pass:
out vec2 v2f_PointUV; // We'll store gl_PointCoord in frag if needed, or pass custom data

// For Object pass:
out vec3 v2fColor;    
out vec3 v2fNormal;   
out vec2 v2fTexCoord; 
out vec3 v2fKa;       
out vec3 v2fKd;       
out vec3 v2fKs;       
out float v2fNs;      
out vec3 v2fKe;       
out vec3 v2fPosition; 

////////////////////////////////////////////////////////////////////////////////
// main()
////////////////////////////////////////////////////////////////////////////////
void main()
{
    if (isParticlePass)
    {
        // *** PARTICLE PASS ***
        // We assume the CPU side sets only 'iPosition' at location=0
        // so we use iPosition for the 3D coords:

        // We do not have the additional attributes like iNormal, iColor, etc.
        // We'll read from iPosition:
        //   If you bound your particle data also at location=0, 
        //   it's the same 'in vec3 iPosition' from above.

        // Transform with (uProj * uView) ...
        // We'll require the CPU to supply those as well:
        vec4 pos = uProj * uView * vec4(iPosition, 1.0);
        gl_Position = pos;
        gl_PointSize = uPointSize;

        // We can pass something to the fragment if we want:
        v2f_PointUV = vec2(0.0, 0.0); // not strictly needed, but example
    }
    else
    {
        // *** NORMAL OBJECT PASS ***

        // Copy input color to output color
        v2fColor = iColor;

        // Apply normal matrix
        v2fNormal = normalize(uNormalMatrix * iNormal);

        // Calculate texture coordinates
        float u = (uDiff.x != 0.0) ? (iPosition.x - uMin.x) / uDiff.x : 0.0;
        float v = (uDiff.y != 0.0) ? (iPosition.z - uMin.y) / uDiff.y : 0.0;
        v2fTexCoord = vec2(clamp(u, 0.0, 1.0), clamp(v, 0.0, 1.0));

        // Pass material
        v2fKa = iKa;
        v2fKd = iKd;
        v2fKs = iKs;
        v2fNs = iNs;
        v2fKe = iKe;

        v2fPosition = iPosition;

        // Transform position
        gl_Position = uProjCameraWorld * vec4(iPosition, 1.0);
    }
}
