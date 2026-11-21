#version 440

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 uv;
layout(location = 0) out vec2 vUv;

layout(std140, binding = 0) uniform buf {
    mat4 matrix;
    float sdrWhite;
    float encodeSdr;
    float opacity;
    float pad;
};

void main()
{
    vUv = uv;
    gl_Position = matrix * vec4(position, 0.0, 1.0);
}
