#version 430 core

uniform sampler2D uTex;

out vec4 outColor;

void main()
{
    // gl_PointCoord: built-in that gives [0..1] coords in the point
    // sprite. We read from the texture's alpha to shape the particle.
    vec2 uv = gl_PointCoord;

    vec4 texColor = texture(uTex, uv);

    // We simply output the texture color. Because we're using
    // additive blending (SRC_ALPHA, ONE), bright areas add up.
    outColor = texColor;
}

