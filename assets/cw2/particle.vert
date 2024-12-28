#version 430 core

layout(location=0) in vec3 inPosition; // The per-particle position from the VBO

uniform mat4 uView;
uniform mat4 uProj;

// If you want a uniform size for all particles:
uniform float uPointSize = 32.0;

void main()
{
    // Transform the position
    gl_Position = uProj * uView * vec4(inPosition, 1.0);
    // Assign the point size
    gl_PointSize = uPointSize;
}
