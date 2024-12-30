#version 430

in vec2 fTexCoord;
in vec4 fColor;

uniform sampler2D uTexture;

out vec4 FragColor;

void main() {
    float alpha = texture(uTexture, fTexCoord).r;
    FragColor = fColor * alpha;
}

