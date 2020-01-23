cmake_minimum_required(VERSION 3.8)
project(PhantasmRenderer)

# =========================================
# global options

# =========================================
# post-process options

# =========================================
# define library

file(GLOB_RECURSE SOURCES "src/*.cc" "src/*.hh")

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" FILES ${SOURCES})

add_library(phantasm-renderer STATIC ${SOURCES})

target_include_directories(phantasm-renderer
    PUBLIC
        src/
)

target_link_libraries(phantasm-renderer PUBLIC
    phantasm-hardware-interface
    clean-core
    typed-geometry
    rich-log
    reflector
)

# =========================================
# set up compile flags

if (MSVC)
    target_compile_options(phantasm-renderer PUBLIC /MP)
else()
    target_compile_options(phantasm-renderer PRIVATE -Wall -fPIC)
    target_link_libraries(phantasm-renderer PUBLIC -fuse-ld=gold)
endif()
