#version 460

layout (location = 0) in ivec2 in_src;
layout (location = 1) in ivec2 in_dst;

layout (location = 0) out vec2 out_src;

// TODO: Use uniforms instead.
const float TILESET_WIDTH = 400.0f;
const float TILESET_HEIGHT = 400.0f;

void main() {
    ivec2 a[4] = {ivec2(0, 0), ivec2(16, 0), ivec2(0, 16), ivec2(16, 16)};
    ivec2 dst = in_dst;
    dst += a[gl_VertexIndex];

    vec2 pos;
    pos.x = float(in_dst.x)/960.0 - 1.0; // TODO: Add uniform variables for window
    pos.y = float(in_dst.y)/540.0 - 1.0; // width and height.

    gl_Position = vec4(pos, 0.0, 1.0);
    out_src = vec2(float(in_src.x) / TILESET_WIDTH, float(in_src.y) / TILESET_HEIGHT);
}