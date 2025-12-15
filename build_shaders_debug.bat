@echo off
glslang -V --target-env vulkan1.1 -C -Od -gVS code/tile.vert -o build/shaders/tile_vert.spv
glslang -V --target-env vulkan1.1 -C -Od -gVS code/tile.frag -o build/shaders/tile_frag.spv
glslang -V --target-env vulkan1.1 -C -Od -gVS code/entity.vert -o build/shaders/entity_vert.spv
glslang -V --target-env vulkan1.1 -C -Od -gVS code/entity.frag -o build/shaders/entity_frag.spv
