cmake_minimum_required(VERSION 4.0.0)
include_directories(vendor vendor/SDL3/include)
link_directories(vendor/SDL3/lib)

project(Platformer)
add_executable(Platformer WIN32 main.c)
target_link_libraries(Platformer SDL3.lib SDL3_image.lib)