#pragma once

namespace pr::backend::assets
{
struct image_size
{
    unsigned width;
    unsigned height;
    unsigned num_mipmaps;
    unsigned array_size;
};

struct image_data
{
    unsigned char* raw;
};

[[nodiscard]] constexpr inline bool is_valid(image_data const& data) { return data.raw != nullptr; }

[[nodiscard]] image_data load_image(char const* filename, image_size& out_size);

[[nodiscard]] unsigned get_num_mip_levels(unsigned width, unsigned height);

void copy_subdata(image_data const& data, void* dest, unsigned stride, unsigned width, unsigned height);

void free(image_data const& data);

}
