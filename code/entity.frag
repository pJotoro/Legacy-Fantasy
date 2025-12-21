#version 460

layout (location = 0) in vec2 in_texture_pos;
layout (location = 1) flat in int in_frame_idx;

layout (location = 0) out vec4 out_color;

layout (binding = 0, set = 1) uniform sampler2DArray sprite;

void main() {
    vec3 pos = vec3(in_texture_pos, float(abs(in_frame_idx)-1));
    out_color = texture(sprite, vec3(pos.x*float(sign(in_frame_idx)), pos.y, pos.z));
}