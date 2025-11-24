#version 460

layout (location = 0) in ivec4 in_rect;
layout (location = 1) in uint in_frame_idx;

layout (location = 0) out vec3 out_pos;

void main() {
    ivec2 a[6] = {ivec2(0, 0), ivec2(1, 0), ivec2(1, 1), ivec2(1, 1), ivec2(0, 1), ivec2(0, 0)};
    
    ivec2 pos = in_rect.xy;
    ivec2 size = in_rect.zw - in_rect.xy;
    size.x *= a[gl_VertexIndex].x;
    size.y *= a[gl_VertexIndex].y;
    pos += size;

    out_pos.x = float(pos.x)/960.0f - 1.0f;
    out_pos.y = float(pos.y)/540.0f - 1.0f;
    out_pos.z = float(in_frame_idx); // TODO: This is almost certainly not correct.
}