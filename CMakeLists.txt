cmake_minimum_required(VERSION 3.8)
project(PhantasmRenderer)

include(cmake/UnityBuild.cmake)
include(cmake/SourceGroup.cmake)

# =========================================
# global options

# =========================================
# post-process options

# =========================================
# define library

file(GLOB_RECURSE SOURCES "src/*.cc")
file(GLOB_RECURSE HEADERS "src/*.hh" "src/*.inl")

arcana_source_group(SOURCES)
arcana_source_group(HEADERS)

add_library(phantasm-renderer STATIC ${SOURCES} ${HEADERS})
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
    dxc-wrapper
)

# =========================================
# set up compile flags

if (MSVC)
    target_compile_options(phantasm-renderer PUBLIC /MP)
else()
    target_compile_options(phantasm-renderer PRIVATE -Wall -fPIC)
    target_link_libraries(phantasm-renderer PUBLIC -fuse-ld=gold)
endif()
