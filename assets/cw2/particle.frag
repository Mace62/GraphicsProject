#version 430

in vec4 fragColor;  // Incoming color from vertex shader

uniform sampler2D uTexture; // Particle texture

out vec4 vColor;

void main() {
    // Sample the texture using the point coordinates (normalized from 0 to 1)
    vec4 texColor = texture(uTexture, gl_PointCoord);
    
    // Multiply the texture's alpha with the fragColor alpha to apply alpha masking
    //texColor.a *= fragColor.a;  // Apply the alpha from the incoming color

    // Apply the texture color and the incoming fragColor (modulate RGB)
    vColor = texColor;

    // Optional: If you want to discard transparent fragments
    if (vColor.a < 1) {
        discard; // Discard fragments with low alpha (adjust threshold as needed)
    }
}
