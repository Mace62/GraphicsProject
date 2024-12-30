#version 430

layout(location = 0) in vec2 vPosition;
layout(location = 1) in vec2 vTexCoord;
layout(location = 2) in vec4 vColor;

layout(location = 0) uniform mat4 projection;  // Add projection matrix uniform

out vec2 fTexCoord;
out vec4 fColor;

void main() {
    gl_Position = projection * vec4(vPosition, 0.0, 1.0);
    fTexCoord = vTexCoord;
    fColor = vColor;
}