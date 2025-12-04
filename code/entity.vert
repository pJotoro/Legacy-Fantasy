#version 460

layout (location = 0) in ivec4 in_rect;
layout (location = 1) in uint in_frame_idx;

layout (location = 0) out vec2 out_texture_pos;
layout (location = 1) flat out uint out_frame_idx;

void main() {
    ivec2 a[6] = {ivec2(0, 0), ivec2(1, 0), ivec2(1, 1), ivec2(1, 1), ivec2(0, 1), ivec2(0, 0)};
    
    ivec2 pos = in_rect.xy;
    ivec2 size = in_rect.zw - in_rect.xy;
    size.x *= a[gl_VertexIndex].x;
    size.y *= a[gl_VertexIndex].y;
    pos += size;

    gl_Position = vec4(float(pos.x)/480.0 - 1.0, float(pos.y)/270.0 - 1.0, 0.0, 1.0);
    out_texture_pos = vec2(a[gl_VertexIndex]);
    out_frame_idx = in_frame_idx;
}