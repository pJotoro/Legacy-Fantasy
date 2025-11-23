#version 460

layout (location = 0) in ivec2 in_src;
layout (location = 1) in ivec2 in_dst;

layout (location = 0) out ivec2 out_src;

void main() {
    ivec2 a[4] = {ivec2(0, 0), ivec2(16, 0), ivec2(0, 16), ivec2(16, 16)};
    ivec2 dst = in_dst;
    dst.x += a[gl_VertexIndex].x;
    dst.y += a[gl_VertexIndex].y;

    vec2 pos;
    pos.x = float(in_dst.x)/960.0 - 1.0; // TODO: Add uniform variablies for window
    pos.y = float(in_dst.y)/540.0 - 1.0; // width and height.

    gl_Position = vec4(pos, 0.0, 1.0);
    out_src = in_src;
}