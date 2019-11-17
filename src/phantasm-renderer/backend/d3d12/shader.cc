#include "shader.hh"

namespace
{
[[maybe_unused]] [[nodiscard]] constexpr char const* get_domain_literal(pr::backend::shader_domain domain)
{
    switch (domain)
    {
    case pr::backend::shader_domain::pixel:
        return "ps_5_1";
    case pr::backend::shader_domain::vertex:
        return "vs_5_1";
    case pr::backend::shader_domain::domain:
        return "ds_5_1";
    case pr::backend::shader_domain::hull:
        return "hs_5_1";
    case pr::backend::shader_domain::geometry:
        return "gs_5_1";
    case pr::backend::shader_domain::compute:
        return "cs_5_1";
    }
}

[[maybe_unused]] [[nodiscard]] constexpr char const* get_default_entrypoint(pr::backend::shader_domain domain)
{
    switch (domain)
    {
    case pr::backend::shader_domain::pixel:
        return "mainPS";
    case pr::backend::shader_domain::vertex:
        return "mainVS";
    case pr::backend::shader_domain::domain:
        return "mainDS";
    case pr::backend::shader_domain::hull:
        return "mainHS";
    case pr::backend::shader_domain::geometry:
        return "mainGS";
    case pr::backend::shader_domain::compute:
        return "mainCS";
    }
}
}

pr::backend::d3d12::shader pr::backend::d3d12::load_binary_shader_from_file(const char* filename, pr::backend::shader_domain domain)
{
    shader res;
    res.domain = domain;
    res.bytecode = pr::backend::detail::unique_buffer::create_from_binary_file(filename);
    return res;
}
