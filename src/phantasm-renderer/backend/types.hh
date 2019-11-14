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
}
