cmake_minimum_required(VERSION 3.8)
project(PhantasmRenderer)

# =========================================
# global options

option(PR_BACKEND_VULKAN "enable Vulkan backend" ON)
option(PR_BACKEND_D3D12 "enable DirectX 12 backend" ON)

# If disabled, cmd::debug_marker is ignored
option(PR_ENABLE_DEBUG_MARKERS "enable API debug markers" ON)

# Enables PIX detection, marker support and capture forcing
# requires WinPixEventRuntime.dll to be available to the executable (in the same folder)
# this dll is included in extern/win32_pix_runtime/bin/
option(PR_ENABLE_D3D12_PIX "enable DirectX 12 PIX integration" OFF)
option(PR_ENABLE_SDL2 "Search for and enable SDL2 integration" ON)

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

target_include_directories(phantasm-renderer
    PUBLIC
        src/
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/extern/renderdoc_app/include/"
)

target_link_libraries(phantasm-renderer PUBLIC
    phantasm-hardware-interface
    clean-core
    typed-geometry
    rich-log
)

if (PR_ENABLE_DEBUG_MARKERS)
    target_compile_definitions(phantasm-renderer PUBLIC PR_ENABLE_DEBUG_MARKERS)
endif()


# =========================================
# set up compile flags

if (MSVC)
    target_compile_options(phantasm-renderer PUBLIC /MP)
else()
    # Pthread
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)

    target_compile_options(phantasm-renderer PRIVATE -Wall -fPIC)
    target_link_libraries(phantasm-renderer PUBLIC -fuse-ld=gold -lX11 PRIVATE Threads::Threads)
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
    target_link_libraries(phantasm-renderer PRIVATE
        "d3d12.lib" "dxgi.lib" "dxguid.lib"
        )

    # PIX Runtime
    if (PR_ENABLE_D3D12_PIX)
        target_link_libraries(phantasm-renderer PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/extern/win32_pix_runtime/bin/WinPixEventRuntime.lib")
        target_include_directories(phantasm-renderer PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/extern/win32_pix_runtime/include/")
        target_compile_definitions(phantasm-renderer PUBLIC PR_BACKEND_D3D12_HAS_PIX USE_PIX)
    endif()

else()
    message(STATUS "[phantasm renderer] D3D12 backend disabled")
endif()

if (MSVC)
    # For device_tentative
    target_link_libraries(phantasm-renderer PRIVATE "Shcore.lib")
endif()

if (PR_ENABLE_SDL2)
    find_package(SDL2)
    if (SDL2_LIBRARIES)
        message(STATUS "[phantasm renderer] SDL2 support enabled")
        target_include_directories(phantasm-renderer PUBLIC ${SDL2_INCLUDE_DIRS})
        target_link_libraries(phantasm-renderer PUBLIC ${SDL2_LIBRARIES})
        target_compile_definitions(phantasm-renderer PUBLIC PR_BACKEND_HAS_SDL2)
    else()
        message(STATUS "[phantasm renderer] SDL2 not found")
    endif()
endif()

if (TARGET reflector)
    target_link_libraries(phantasm-renderer PUBLIC reflector)
    target_compile_definitions(phantasm-renderer PUBLIC PR_BACKEND_HAS_REFLECTOR)
    message(STATUS "[phantasm renderer] reflector found")
endif()
