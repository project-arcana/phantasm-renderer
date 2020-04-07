#pragma once

#include <clean-core/vector.hh>

#include <phantasm-hardware-interface/arguments.hh>

#include <phantasm-renderer/common/hashable_storage.hh>
#include <phantasm-renderer/common/state_info.hh>

#include <phantasm-renderer/fwd.hh>
#include <phantasm-renderer/resource_types.hh>

namespace pr
{
class Context;

// fixed size, hashable, no raw resources allowed
struct argument
{
public:
    void add(texture const& img);
    void add(buffer const& buffer);
    void add(render_target const& rt);

    void add_mutable(texture const& img);
    void add_mutable(buffer const& buffer);
    void add_mutable(render_target const& rt);

    void add_sampler(phi::sampler_filter filter, unsigned anisotropy = 16u)
    {
        auto& new_sampler = _info.get().samplers.emplace_back();
        new_sampler.init_default(filter, anisotropy);
    }

    void add_sampler(phi::sampler_config const& config) { _info.get().samplers.push_back(config); }

private:
    friend struct argument_builder;
    static void populate_srv(phi::resource_view& new_rv, pr::texture const& img);
    static void populate_uav(phi::resource_view& new_rv, pr::texture const& img);

private:
    friend class raii::Frame;
    friend class Context;
    hashable_storage<shader_view_info> _info;
};

struct prebuilt_argument
{
    phi::handle::shader_view _sv = phi::handle::null_shader_view;
    phi::handle::resource _cbv = phi::handle::null_resource;
    unsigned _cbv_offset = 0;
};

using auto_prebuilt_argument = auto_destroyer<prebuilt_argument, false>;

// builder, only received directly from Context, can grow indefinitely in size, not hashable
struct argument_builder
{
public:
    // add SRVs

    argument_builder& add(texture const& img)
    {
        auto& new_rv = _srvs.emplace_back();
        argument::populate_srv(new_rv, img);
        return *this;
    }
    argument_builder& add(buffer const& buffer)
    {
        auto& new_rv = _srvs.emplace_back();
        new_rv.init_as_structured_buffer(buffer.res.handle, buffer.info.size_bytes / buffer.info.stride_bytes, buffer.info.stride_bytes);
        return *this;
    }
    argument_builder& add(render_target const& rt)
    {
        auto& new_rv = _srvs.emplace_back();
        new_rv.init_as_tex2d(rt.res.handle, rt.info.format, rt.info.num_samples > 1);
        return *this;
    }
    argument_builder& add(phi::resource_view const& raw_rv)
    {
        _srvs.push_back(raw_rv);
        return *this;
    }

    // add UAVs

    argument_builder& add_mutable(texture const& img)
    {
        auto& new_rv = _uavs.emplace_back();
        argument::populate_uav(new_rv, img);
        return *this;
    }
    argument_builder& add_mutable(buffer const& buffer)
    {
        auto& new_rv = _uavs.emplace_back();
        new_rv.init_as_structured_buffer(buffer.res.handle, buffer.info.size_bytes / buffer.info.stride_bytes, buffer.info.stride_bytes);
        return *this;
    }
    argument_builder& add_mutable(render_target const& rt)
    {
        auto& new_rv = _uavs.emplace_back();
        new_rv.init_as_tex2d(rt.res.handle, rt.info.format, rt.info.num_samples > 1);
        return *this;
    }
    argument_builder& add_mutable(phi::resource_view const& raw_rv)
    {
        _uavs.push_back(raw_rv);
        return *this;
    }

    // add samplers

    argument_builder& add_sampler(phi::sampler_filter filter, unsigned anisotropy = 16u)
    {
        auto& new_sampler = _samplers.emplace_back();
        new_sampler.init_default(filter, anisotropy);
        return *this;
    }

    argument_builder& add_sampler(phi::sampler_config const& config)
    {
        _samplers.push_back(config);
        return *this;
    }

    // finalize

    [[nodiscard]] auto_prebuilt_argument make_graphics();
    [[nodiscard]] auto_prebuilt_argument make_compute();

public:
    friend class Context;
    argument_builder(Context* parent) : _parent(parent)
    {
        _srvs.reserve(8);
        _uavs.reserve(8);
        _samplers.reserve(4);
    }
    Context* const _parent;

    cc::vector<phi::resource_view> _srvs;
    cc::vector<phi::resource_view> _uavs;
    cc::vector<phi::sampler_config> _samplers;
};

}
