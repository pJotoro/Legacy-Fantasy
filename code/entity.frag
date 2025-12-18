#version 460

layout (location = 0) in vec2 in_texture_pos;
layout (location = 1) flat in uint in_frame_idx;

layout (location = 0) out vec4 out_color;

layout (binding = 1) uniform sampler2DArray sprite;

void main() {
    vec3 pos = vec3(in_texture_pos, float(in_frame_idx));
    out_color = texture(sprite, pos);
}