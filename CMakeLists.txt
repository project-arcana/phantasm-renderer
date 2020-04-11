cmake_minimum_required(VERSION 3.8)
project(PhantasmRenderer)

if (NOT TARGET clean-core)
    message(FATAL_ERROR "[phantasm-renderer] clean-core must be available")
endif()

# =========================================
# define library

file(GLOB_RECURSE SOURCES "src/*.cc")
file(GLOB_RECURSE HEADERS "src/*.hh" "src/*.inl")

arcana_add_library(PR phantasm-renderer SOURCES HEADERS)

target_include_directories(phantasm-renderer PUBLIC src/)

target_link_libraries(phantasm-renderer PUBLIC
    phantasm-hardware-interface
    clean-core
    typed-geometry
    rich-log
    reflector
    dxc-wrapper
)
