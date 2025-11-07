cmake_minimum_required(VERSION 4.0.0)

set(CMAKE_C_STANDARD 17)
include_directories(libraries libraries/SDL3/include libraries/SDL3/include/SDL3)
link_directories(libraries/SDL3/lib)

project(LegacyFantasy)
add_executable(LegacyFantasy WIN32 code/main.c code/libraries.c)
target_link_libraries(LegacyFantasy SDL3.lib SDL3_ttf.lib)
target_compile_options(LegacyFantasy PRIVATE /W4 /WX /wd4456 /diagnostics:column)
add_link_options(LegacyFantasy)