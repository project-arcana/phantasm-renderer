cmake_minimum_required(VERSION 3.18)
project(PhantasmRenderer)

if (NOT TARGET clean-core)
    message(FATAL_ERROR "[phantasm-renderer] clean-core must be available")
endif()

# =========================================
# options

option(PR_ENABLE_SHADER_COMPILATION "If enabled, allow runtime shader compilation by linking to DXC" ON)

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
)

if (PR_ENABLE_SHADER_COMPILATION)
    target_link_libraries(phantasm-renderer PUBLIC dxc-wrapper)
    target_compile_definitions(phantasm-renderer PUBLIC PR_HAS_DXC)
endif()

# =========================================
# set up compile flags

if (MSVC)
    target_compile_options(phantasm-renderer PRIVATE /WX)
endif()
