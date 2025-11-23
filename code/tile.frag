#version 460

layout (location = 0) in vec2 in_src;
layout (location = 0) out vec4 out_color;

layout (binding = 0) uniform sampler2D tileset;

void main() {
    out_color = texture(tileset, in_src);
}