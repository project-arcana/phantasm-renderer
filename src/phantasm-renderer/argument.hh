#pragma once

#include <clean-core/vector.hh>

#include <phantasm-hardware-interface/arguments.hh>

#include <phantasm-renderer/resource_types.hh>

namespace pr
{
struct shader_view
{
public:
    shader_view()
    {
        _srvs.reserve(8);
        _uavs.reserve(8);
        _resource_guids.reserve(_srvs.size() + _uavs.size());
        _samplers.reserve(4);
    }

    void add_srv(image const& img);
    void add_srv(buffer const& buffer);
    void add_srv(render_target const& rt);

    void add_uav(image const& img);
    void add_uav(buffer const& buffer);
    void add_uav(render_target const& rt);

    void add_sampler(phi::sampler_filter filter, unsigned anisotropy = 16u)
    {
        auto& new_sampler = _samplers.emplace_back();
        new_sampler.init_default(filter, anisotropy);
    }

    void add_sampler(phi::sampler_config const& config) { _samplers.push_back(config); }

public:
    cc::vector<phi::resource_view> _srvs;
    cc::vector<phi::resource_view> _uavs;
    cc::vector<uint64_t> _resource_guids;
    cc::vector<phi::sampler_config> _samplers;
};

struct baked_shader_view_data
{
    phi::handle::shader_view _sv;
    void destroy(pr::Context* ctx);
};

using baked_shader_view = raii_handle<baked_shader_view_data>;

}
