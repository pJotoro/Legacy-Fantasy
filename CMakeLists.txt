cmake_minimum_required(VERSION 4.0 FATAL_ERROR)
cmake_policy(VERSION 4.0)

set(CMAKE_C_STANDARD 17) # This isn't *technically* the right way to do things. You're supposed to set the standard as a property of the target. However, it doesn't matter, because we only have one target.

find_package(Vulkan 1.1.0 REQUIRED)
set(VULKAN_SDK "$ENV{VULKAN_SDK}")
if(EXISTS "${VULKAN_SDK}/Include/Volk/volk.h" AND EXISTS "${VULKAN_SDK}/Include/Volk/volk.c")
	message(STATUS "Successfully found volk.")
else()
	message(FATAL_ERROR "Failed to find Volk. It can be installed using the maintenance tool found at C:\\VulkanSDK\\...\\maintenancetool.exe")
endif()

include_directories(libraries libraries/SDL3 "${VULKAN_SDK}/Include")

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
