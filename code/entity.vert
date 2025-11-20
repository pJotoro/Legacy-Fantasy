#version 450

layout (location = 0) in ivec2 pos;
layout (location = 1) in uint frame_idx;

void main() {

    gl_Position = vec4(pos, 0.0, 1.0);
}