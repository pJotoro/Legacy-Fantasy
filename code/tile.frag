#version 460

layout (location = 0) in vec4 in_src;
layout (location = 0) out vec4 out_color;

layout (binding = 0) uniform sampler2D tileset;

void main() {
    out_color = vec4(0.0, 0.0, 0.0, 0.0); // TODO
}