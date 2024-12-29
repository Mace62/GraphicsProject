#version 330 core
layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;
layout(location = 2) in float size;

uniform mat4 uViewProjection;

out vec4 vColor;

void main() {
    gl_Position = uViewProjection * vec4(position, 1.0);
    gl_PointSize = size / gl_Position.w; // This makes the size perspective-correct
    vColor = color;
}