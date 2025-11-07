@echo off
glslang -V --target-env vulkan1.1 -C code/shader.vert -o build/shaders/vert.spv
glslang -V --target-env vulkan1.1 -C code/shader.frag -o build/shaders/frag.spv
