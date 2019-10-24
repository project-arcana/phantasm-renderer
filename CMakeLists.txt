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

find_package(Vulkan)
if (Vulkan_FOUND)
    message(STATUS "[phantasm renderer] Vulkan backend enabled")
    target_compile_definitions(phantasm-renderer PUBLIC PR_BACKEND_VULKAN)
    target_link_libraries(phantasm-renderer PRIVATE ${Vulkan_LIBRARY})
    target_include_directories(phantasm-renderer PRIVATE ${Vulkan_INCLUDE_DIRS})
else()
    message(STATUS "[phantasm renderer] Vulkan backend disabled")
endif()

if (WIN32)
    message(STATUS "[phantasm renderer] D3D12 backend enabled")
    target_compile_definitions(phantasm-renderer PUBLIC PR_BACKEND_D3D12)
    target_link_libraries(phantasm-renderer PRIVATE "d3d12.lib" "dxgi.lib" "d3dcompiler.lib")
else()
    message(STATUS "[phantasm renderer] D3D12 backend disabled")
endif()
