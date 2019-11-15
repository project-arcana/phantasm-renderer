#pragma once

#include <variant>

#include <clean-core/native/win32_fwd.hh>

#include <dxgiformat.h>

namespace pr::backend::d3d12::img
{
struct image_info
{
    unsigned width;
    unsigned height;
    unsigned depth;
    unsigned array_size;
    unsigned num_mip_levels;
    DXGI_FORMAT format;
};

struct image_handle
{
    bool is_dds = true;
    union {
        HANDLE handle;
        unsigned char* stbi_data;
    };
};

[[nodiscard]] constexpr inline bool is_valid(image_handle const& handle)
{
    return handle.is_dds ? handle.handle != nullptr : handle.stbi_data != nullptr;
}

// unused image loading, supports DDS and advanced features
[[nodiscard]] image_handle load_image(char const* filename, image_info& out_info);

void copy_pixels(image_handle const& handle, void* dest, unsigned stride, unsigned width, unsigned height);
void free(image_handle const& handle);
}
