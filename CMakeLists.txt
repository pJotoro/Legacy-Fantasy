cmake_minimum_required(VERSION 4.0.0)
set(CMAKE_C_STANDARD 17)
include_directories(vendor vendor/SDL3/include vendor/SDL3/include/SDL3)
link_directories(vendor/SDL3/lib)

project(LegacyFantasy)
add_executable(LegacyFantasy WIN32 main.c libraries.c)
target_link_libraries(LegacyFantasy SDL3.lib SDL3_image.lib SDL3_ttf.lib)
target_compile_options(LegacyFantasy PRIVATE /W4 /WX /diagnostics:column)