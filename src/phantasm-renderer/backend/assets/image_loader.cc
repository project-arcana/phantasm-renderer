#include "image_loader.hh"

#include <cstdint>
#include <cstring>

#include <clean-core/assert.hh>
#include <clean-core/utility.hh>

#include <typed-geometry/tg.hh>

#include <phantasm-renderer/backend/detail/stb_image.hh>

namespace
{
void compute_mip(unsigned char* data, unsigned width, unsigned height)
{
    constexpr unsigned offsetsX[] = {0, 1, 0, 1};
    constexpr unsigned offsetsY[] = {0, 0, 1, 1};

    auto* const image_data = reinterpret_cast<uint32_t*>(data);

    auto const get_byte = [](auto color, int component) { return (color >> (8 * component)) & 0xff; };
    auto const get_color = [&](unsigned x, unsigned y) { return image_data[x + y * width]; };
    auto const set_color = [&](unsigned x, unsigned y, auto color) { image_data[x + y * width / 2] = color; };

    for (auto y = 0u; y < height; y += 2)
    {
        for (auto x = 0u; x < width; x += 2)
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
    int width, height, num_channels;
    auto* const res_data = ::stbi_load(filename, &width, &height, &num_channels, 4);

    if (!res_data)
        return {nullptr};

    out_size.width = unsigned(width);
    out_size.height = unsigned(height);
    out_size.num_mipmaps = get_num_mip_levels(out_size.width, out_size.height);
    out_size.array_size = 1;
    return {res_data};
}

unsigned pr::backend::assets::get_num_mip_levels(unsigned width, unsigned height)
{
    return static_cast<unsigned>(tg::floor(tg::log2(static_cast<float>(cc::max(width, height))))) + 1u;
}

void pr::backend::assets::copy_subdata(const pr::backend::assets::image_data& data, void* dest, unsigned stride, unsigned width, unsigned height)
{
    for (auto y = 0u; y < unsigned(height); ++y)
    {
        std::memcpy(static_cast<char*>(dest) + y * unsigned(stride), data.raw + y * unsigned(width), unsigned(width));
    }

    compute_mip(data.raw, width / 4, height);
}

void pr::backend::assets::free(const pr::backend::assets::image_data& data)
{
    if (data.raw)
        ::stbi_image_free(data.raw);
}
