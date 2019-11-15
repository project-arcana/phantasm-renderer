#pragma once

namespace pr::backend::assets
{
struct image_size
{
    int width;
    int height;
    int num_mipmaps;
};

struct image_data
{
    unsigned char* raw;
};

[[nodiscard]] constexpr inline bool is_valid(image_data const& data) { return data.raw != nullptr; }

[[nodiscard]] image_data load_image(char const* filename, image_size& out_size);

[[nodiscard]] int get_num_mip_levels(int width, int height);

void copy_subdata(image_data const& data, void* dest, int stride, int width, int height);

void free(image_data const& data);

}
