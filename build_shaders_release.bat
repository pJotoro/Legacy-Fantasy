@echo off

glslang -V --target-env vulkan1.1 -C code/tile.vert -o build/shaders/tile_vert.spv
glslang -V --target-env vulkan1.1 -C code/tile.frag -o build/shaders/tile_frag.spv
glslang -V --target-env vulkan1.1 -C code/entity.vert -o build/shaders/entity_vert.spv
glslang -V --target-env vulkan1.1 -C code/entity.frag -o build/shaders/entity_frag.spv
glslang -V --target-env vulkan1.1 -C code/triangle.vert -o build/shaders/triangle_vert.spv
glslang -V --target-env vulkan1.1 -C code/triangle.frag -o build/shaders/triangle_frag.spv