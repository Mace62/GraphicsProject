#version 430

layout(location = 0) in vec2 aPosition;
layout(location = 1) in vec4 aColor;

layout(location = 0) uniform vec2 screenDims;

out vec4 vColor;

void main() {
    gl_Position = vec4(2.0 * aPosition.x / screenDims.x - 1.0, 
                      2.0 * aPosition.y / screenDims.y - 1.0,
                      0.0, 1.0);
    vColor = aColor;
}