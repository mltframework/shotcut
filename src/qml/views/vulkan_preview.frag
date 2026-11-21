#version 440

layout(location = 0) in vec2 vUv;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 matrix;
    float sdrWhite;
    float encodeSdr;
    float opacity;
    float pad;
};

layout(binding = 1) uniform sampler2D tex;

// Inverse of the BT.1886 EOTF used when the frame was linearized. The player
// target is an 8-bit UNORM texture, so these are the code values Qt composites.
vec3 linearToBt1886(vec3 x)
{
    return pow(clamp(x, vec3(0.0), vec3(1.0)), vec3(1.0 / 2.4));
}

void main()
{
    vec4 c = texture(tex, vUv);
    vec3 rgb = max(c.rgb, vec3(0.0));
    // scRGB stores SDR white (203 nits) at 203/80. Divide that back to 1.0 and
    // encode it. Reinhard is not the inverse EOTF, so those values are too
    // dark in an 8-bit buffer. The HDR window samples the same image unchanged.
    if (encodeSdr > 0.5)
        rgb = linearToBt1886(rgb / max(sdrWhite, 0.001));
    fragColor = vec4(rgb, c.a * opacity);
}
