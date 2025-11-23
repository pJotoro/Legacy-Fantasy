#version 460

layout (location = 0) in vec2 in_src;
layout (location = 0) out vec4 out_color;

layout (binding = 0) uniform sampler2DArray tileset;

void main() {
    vec3 src = vec3(in_src, 0.0f);
    out_color = texture(tileset, src);
}