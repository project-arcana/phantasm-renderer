cmake_minimum_required(VERSION 3.8)
project(PhantasmRenderer)

# =========================================
# global options

option(PR_BACKEND_VULKAN "enable Vulkan backend" ON)
option(PR_BACKEND_D3D12 "enable DirectX 12 backend" ON)


# =========================================
# post-process options

find_package(Vulkan)

if (NOT Vulkan_FOUND)
    set(PR_BACKEND_VULKAN OFF)
endif()

if (NOT MSVC)
    set(PR_BACKEND_D3D12 OFF)
endif()


# =========================================
# define library

file(GLOB_RECURSE SOURCES "src/*.cc" "src/*.hh")

# disable backends
if (NOT PR_BACKEND_D3D12)
    foreach(src ${SOURCES})
        if (${src} MATCHES "phantasm-renderer/backend/d3d12/")
            list(REMOVE_ITEM SOURCES ${src})
        endif()
    endforeach()
endif()
if (NOT PR_BACKEND_VULKAN)
    foreach(src ${SOURCES})
        if (${src} MATCHES "phantasm-renderer/backend/vulkan/")
            list(REMOVE_ITEM SOURCES ${src})
        endif()
    endforeach()
endif()

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" FILES ${SOURCES})

add_library(phantasm-renderer STATIC ${SOURCES})

target_include_directories(phantasm-renderer PUBLIC src/)

target_link_libraries(phantasm-renderer PUBLIC
    clean-core
    typed-geometry
    reflector
    polymesh
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

if (PR_BACKEND_VULKAN)
    message(STATUS "[phantasm renderer] Vulkan backend enabled")
    target_compile_definitions(phantasm-renderer PUBLIC PR_BACKEND_VULKAN)
    target_link_libraries(phantasm-renderer PUBLIC ${Vulkan_LIBRARY})
    target_include_directories(phantasm-renderer PUBLIC ${Vulkan_INCLUDE_DIRS})

    if (NOT MSVC)
        # Non-windows targets require libdl to load Vulkan
        target_link_libraries(phantasm-renderer PRIVATE -ldl)
    endif()
else()
    message(STATUS "[phantasm renderer] Vulkan backend disabled")
endif()

if (PR_BACKEND_D3D12)
    message(STATUS "[phantasm renderer] D3D12 backend enabled")
    target_compile_definitions(phantasm-renderer PUBLIC PR_BACKEND_D3D12)
    target_link_libraries(phantasm-renderer PRIVATE "d3d12.lib" "dxgi.lib" "d3dcompiler.lib")
else()
    message(STATUS "[phantasm renderer] D3D12 backend disabled")
endif()
