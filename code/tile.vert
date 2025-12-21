#version 460

layout (location = 0) in ivec2 in_src;
layout (location = 1) in ivec2 in_dst;

layout (location = 0) out vec2 out_src;

layout (binding = 0, set = 0) uniform Uniforms {
    ivec2 viewport_size;
    ivec2 tileset_size;
    int tile_size;
} uniforms;

void main() {
    ivec2 a[6] = {ivec2(0, 0), ivec2(0, uniforms.tile_size), ivec2(uniforms.tile_size, uniforms.tile_size), ivec2(uniforms.tile_size, uniforms.tile_size), ivec2(uniforms.tile_size, 0), ivec2(0, 0)};
    
    ivec2 dst = in_dst;
    dst += a[gl_VertexIndex];
    vec2 pos;
    pos.x = float(dst.x)/float(uniforms.viewport_size.x) - 1.0;
    pos.y = float(dst.y)/float(uniforms.viewport_size.y) - 1.0;
    
    ivec2 src = in_src;
    src += a[gl_VertexIndex];

    gl_Position = vec4(pos, 0.0, 1.0);
    out_src = vec2(float(src.x) / float(uniforms.tileset_size.x), float(src.y) / float(uniforms.tileset_size.y));
}