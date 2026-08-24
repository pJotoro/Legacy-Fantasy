# Why set such a low version?
# In general, you should never force someone to always update to the latest version of third-party software just to use yours.
# Instead, you should select a version as low as possible that will still work for whatever you're doing.
# In this case, 3.18 is required in order to use the REQUIRED flag in find_library.
cmake_minimum_required(VERSION 4.0 FATAL_ERROR)
cmake_policy(VERSION 4.0)

set(CMAKE_C_STANDARD 17) # This isn't *technically* the right way to do things. You're supposed to set the standard as a property of the target. However, it doesn't matter, because we only have one target.
include_directories(libraries libraries/SDL3)
find_library(SDL3_lib NAMES SDL3.lib PATHS libraries/SDL3/lib REQUIRED)

project(LegacyFantasy)
add_executable(LegacyFantasy WIN32 code/main.c code/libraries.c)
target_link_libraries(LegacyFantasy ${SDL3_lib})

if(MSVC)
	target_compile_options(LegacyFantasy PRIVATE /W4 /WX /wd4456 /wd4552 /wd4553 /wd4127 /diagnostics:column)
	target_link_options(LegacyFantasy PRIVATE /subsystem:windows)
else()
	 message(FATAL_ERROR "TODO: Implement other platforms." )
endif()
