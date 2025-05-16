cmake_minimum_required(VERSION 4.0.0)
include_directories(vendor vendor/SDL3/include)

project(Platformer)
add_executable(Platformer WIN32 main.c)
target_link_libraries(Platformer C:/Projects/Code/Platformer/vendor/SDL3/lib/SDL3.lib C:/Projects/Code/Platformer/vendor/SDL3/lib/SDL3_image.lib)