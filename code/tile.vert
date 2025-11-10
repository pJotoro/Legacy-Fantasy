#version 450

layout (location = 0) in ivec4 in_tile;

void main() {
    ivec2 src = in_tile.xy;
    ivec2 dst = in_tile.zw;

    vec2 pos;
    pos.x = float(dst.x)/960.0 - 1.0; // TODO: Add uniform variablies for window
    pos.y = float(dst.y)/540.0 - 1.0; // width and height.

    // TODO: Add sampler2d for tileset image and render based on that and the
    // src variable.

    gl_Position = vec4(pos, 0.0, 1.0);
}