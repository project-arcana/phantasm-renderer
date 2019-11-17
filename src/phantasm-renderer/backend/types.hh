#pragma once

namespace pr::backend
{
using index_t = int;

struct device_handle
{
    index_t index;
};

enum class shader_domain
{
    pixel,
    vertex,
    domain,
    hull,
    geometry,
    compute
};

namespace assets
{
enum class attribute_format
{
    rgba32f,
    rgb32f,
    rg32f,
    r32f,
    rgba32i,
    rgb32i,
    rg32i,
    r32i,
    rgba32u,
    rgb32u,
    rg32u,
    r32u,
    rgba16i,
    rgb16i,
    rg16i,
    r16i,
    rgba16u,
    rgb16u,
    rg16u,
    r16u,
    rgba8i,
    rgb8i,
    rg8i,
    r8i,
    rgba8u,
    rgb8u,
    rg8u,
    r8u
};
}
}
