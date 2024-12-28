#version 430 core
in vec2 TexCoords;
in vec4 Color;
out vec4 FragColor;

uniform sampler2D fontTex;

void main() {
    float alpha = texture(fontTex, TexCoords).r;
    FragColor = vec4(Color.rgb, Color.a * alpha);
}
