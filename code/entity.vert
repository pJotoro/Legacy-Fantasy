#version 460

layout (location = 0) in ivec4 rect;
layout (location = 1) in uint frame_idx;

void main() {

    gl_Position = vec4(rect.xy, 0.0, 1.0);
}