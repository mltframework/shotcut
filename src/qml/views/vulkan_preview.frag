#version 440

layout(location = 0) in vec2 vUv;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 matrix;
    float sdrWhite;
    float toneMap;
    float opacity;
    float pad;
};

layout(binding = 1) uniform sampler2D tex;

void main()
{
    vec4 c = texture(tex, vUv);
    vec3 rgb = max(c.rgb, vec3(0.0));
    // scRGB stores SDR white (203 nits) at 203/80. The sRGB swapchain encodes
    // linear 0-1, so the SDR player compresses that into 0-1. The HDR window
    // samples the same image unchanged.
    if (toneMap > 0.5) {
        vec3 x = rgb / max(sdrWhite, 0.001);
        rgb = (2.0 * x) / (vec3(1.0) + x);
    }
    fragColor = vec4(rgb, c.a * opacity);
}
