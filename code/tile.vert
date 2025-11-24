// TODO: Use uniforms instead of constants for TILESET_WIDTH, TILESET_HEIGHT, TILE_SIZE, WINDOW_WIDTH, and WINDOW_HEIGHT.

#version 460

layout (location = 0) in ivec2 in_src;
layout (location = 1) in ivec2 in_dst;

layout (location = 0) out vec2 out_src;

const float TILESET_WIDTH = 400.0f;
const float TILESET_HEIGHT = 400.0f;

void main() {
    ivec2 a[6] = {ivec2(0, 0), ivec2(16, 0), ivec2(16, 16), ivec2(16, 16), ivec2(0, 16), ivec2(0, 0)};
    ivec2 dst = in_dst;
    dst += a[gl_VertexIndex];

    vec2 pos;
    pos.x = float(dst.x)/960.0 - 1.0;
    pos.y = float(dst.y)/540.0 - 1.0;

    gl_Position = vec4(pos, 0.0, 1.0);
    out_src = vec2(float(in_src.x) / TILESET_WIDTH, float(in_src.y) / TILESET_HEIGHT);
}