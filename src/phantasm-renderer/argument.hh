#pragma once

#include <clean-core/vector.hh>

#include <phantasm-hardware-interface/arguments.hh>

#include <phantasm-renderer/resource_types.hh>

namespace pr
{
struct argument
{
public:
    argument()
    {
        _srvs.reserve(8);
        _uavs.reserve(8);
        _resource_guids.reserve(_srvs.size() + _uavs.size());
        _samplers.reserve(4);
    }

    void add(image const& img);
    void add(buffer const& buffer);
    void add(render_target const& rt);

    void add_mutable(image const& img);
    void add_mutable(buffer const& buffer);
    void add_mutable(render_target const& rt);

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

struct baked_argument_data
{
    phi::handle::shader_view _sv;
    phi::handle::resource _cbv;
    unsigned _cbv_offset;
    void destroy(pr::Context* ctx);
};

using baked_argument = raii_handle<baked_argument_data>;

}
