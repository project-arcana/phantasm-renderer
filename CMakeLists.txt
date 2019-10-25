cmake_minimum_required(VERSION 3.8)
project(PhantasmRenderer)

# =========================================
# global options


# =========================================
# define library

file(GLOB_RECURSE SOURCES "src/*.cc" "src/*.hh")

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" FILES ${SOURCES})

add_library(phantasm-renderer STATIC ${SOURCES})

target_include_directories(phantasm-renderer PUBLIC src/)

target_link_libraries(phantasm-renderer PUBLIC
    clean-core
    typed-geometry
)


# =========================================
# set up compile flags

if (MSVC)
    target_compile_options(phantasm-renderer PUBLIC /MP)
else()
    target_compile_options(phantasm-renderer PRIVATE -Wall -fPIC)
    target_link_libraries(phantasm-renderer PUBLIC -fuse-ld=gold)
endif()


# =========================================
# optional dependencies

if (TARGET volk)
    if ("$ENV{VULKAN_SDK}" STREQUAL "")
        message(FATAL_ERROR "VULKAN_SDK environment variable not set")
    else()
        message(STATUS "Using vulkan SDK at $ENV{VULKAN_SDK}")
    endif()

    target_link_libraries(typed-graphics PRIVATE volk)
    target_compile_definitions(typed-graphics PUBLIC TG_BACKEND_VULKAN)
endif()
