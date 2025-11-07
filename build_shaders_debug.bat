@echo off
glslang -V --target-env vulkan1.1 -C -Od -g code/shader.vert -o build/shaders/vert.spv
glslang -V --target-env vulkan1.1 -C -Od -g code/shader.frag -o build/shaders/frag.spv
