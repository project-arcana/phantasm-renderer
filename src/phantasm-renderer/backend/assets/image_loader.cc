#include "image_loader.hh"

#include <cstdint>
#include <cstring>

#include <clean-core/assert.hh>

#include <phantasm-renderer/backend/detail/stb_image.hh>

namespace
{
void compute_mip(unsigned char* data, int width, int height)
{
    constexpr int offsetsX[] = {0, 1, 0, 1};
    constexpr int offsetsY[] = {0, 0, 1, 1};

    auto* const image_data = reinterpret_cast<uint32_t*>(data);

    auto const get_byte = [](auto color, int component) { return (color >> (8 * component)) & 0xff; };
    auto const get_color = [&](int x, int y) { return image_data[unsigned(x + y * int(width))]; };
    auto const set_color = [&](int x, int y, auto color) { image_data[x + y * width / 2] = color; };

    for (auto y = 0; y < height; y += 2)
    {
        for (auto x = 0; x < width; x += 2)
        {
            uint32_t color = 0;
            for (auto c = 0; c < 4; ++c)
            {
                uint32_t color_component = 0;
                for (auto i = 0; i < 4; ++i)
                    color_component += get_byte(get_color(x + offsetsX[i], y + offsetsY[i]), 3 - c);

                color = (color << 8) | (color_component / 4);
            }
            set_color(x / 2, y / 2, color);
        }
    }
}
}


pr::backend::assets::image_data pr::backend::assets::load_image(const char* filename, pr::backend::assets::image_size& out_size)
{
    int num_channels;
    auto* const res_data = ::stbi_load(filename, &out_size.width, &out_size.height, &num_channels, 4);

    if (!res_data)
        return {nullptr};

    out_size.num_mipmaps = get_num_mip_levels(out_size.width, out_size.height);
    return {res_data};
}

int pr::backend::assets::get_num_mip_levels(int w, int h)
{
    auto width = unsigned(w);
    auto height = unsigned(h);

    auto res = 0;
    while (true)
    {
        ++res;
        if (width > 1)
            width >>= 1;
        if (height > 1)
            height >>= 1;
        if (width == 1 && height == 1)
            break;
    }
    return res;
}

void pr::backend::assets::copy_subdata(const pr::backend::assets::image_data& data, void* dest, int stride, int width, int height)
{
    for (auto y = 0u; y < unsigned(height); ++y)
    {
        std::memcpy(static_cast<char*>(dest) + y * unsigned(stride), data.raw + y * unsigned(width), unsigned(width));
    }

    compute_mip(data.raw, width / 4, int(height));
}

void pr::backend::assets::free(const pr::backend::assets::image_data& data)
{
    if (data.raw)
        ::stbi_image_free(data.raw);
}