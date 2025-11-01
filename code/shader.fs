#version 450

layout (location = 0) in vec3 in_frag;

layout (location = 0) out vec4 out_color;

void main() {
    out_color = vec4(in_frag, 1.0);
}