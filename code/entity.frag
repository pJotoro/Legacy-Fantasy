#version 460

layout (location = 0) in vec3 in_pos;

layout (location = 0) out vec4 out_color;

layout (binding = 0) uniform sampler2DArray sprite;

void main() {
    out_color = texture(sprite, in_pos);
}