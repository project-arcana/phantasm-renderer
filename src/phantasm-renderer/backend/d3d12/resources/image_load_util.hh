#pragma once

#include <clean-core/native/win32_fwd.hh>

#include <dxgiformat.h>

namespace pr::backend::d3d12::img
{
struct image_info
{
    unsigned width;
    unsigned height;
    unsigned depth;
    unsigned arraySize;
    unsigned mipMapCount;
    DXGI_FORMAT format;
    unsigned bitCount;
};

struct image_handle
{
    HANDLE handle;
};

[[nodiscard]] constexpr inline bool is_valid(image_handle const& handle) { return handle.handle != nullptr; }

[[nodiscard]] image_handle load_image(char const* filename, image_info& out_info);

void copy_pixels(image_handle const& handle, void* dest, unsigned stride, unsigned width, unsigned height);

}
