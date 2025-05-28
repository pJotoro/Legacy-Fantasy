set(CMAKE_DEBUG_POSTFIX d)

cmake_minimum_required(VERSION 4.0.0)
include_directories(vendor vendor/SDL3/include)

project(Platformer)
add_executable(Platformer WIN32 main.c)
set_target_properties(Platformer PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})
target_link_libraries(Platformer C:/Projects/Code/Legacy-Fantasy/vendor/SDL3/lib/SDL3.lib C:/Projects/Code/Legacy-Fantasy/vendor/SDL3/lib/SDL3_image.lib)