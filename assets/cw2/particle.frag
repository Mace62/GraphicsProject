#version 430

uniform sampler2D uTexture; // Particle texture

out vec4 vColor;

void main() {
    // Sample the texture using the point coordinates (normalized from 0 to 1)
    vec4 texColor = texture(uTexture, gl_PointCoord);

    // Apply the texture color and the incoming fragColor (modulate RGB)
    vColor = texColor;

    // Discard transparent fragments
    if (texColor.a < 1) {
        discard;
    }
}
