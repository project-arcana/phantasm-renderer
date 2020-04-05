#pragma once

#include <clean-core/vector.hh>

#include <phantasm-hardware-interface/arguments.hh>

#include <phantasm-renderer/common/hashable_storage.hh>
#include <phantasm-renderer/common/state_info.hh>

#include <phantasm-renderer/resource_types.hh>
#include <phantasm-renderer/fwd.hh>

namespace pr
{
class Context;

// fixed size, hashable
struct argument
{
public:
    void add(image const& img);
    void add(buffer const& buffer);
    void add(render_target const& rt);

    void add_mutable(image const& img);
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
    static void populate_srv(phi::resource_view& new_rv, pr::image const& img);
    static void populate_uav(phi::resource_view& new_rv, pr::image const& img);

private:
    friend class raii::Frame;
    hashable_storage<shader_view_info> _info;
};

struct prebuilt_argument_data
{
    phi::handle::shader_view _sv = phi::handle::null_shader_view;
    phi::handle::resource _cbv = phi::handle::null_resource;
    unsigned _cbv_offset = 0;
    void destroy(pr::Context* ctx);
};

using prebuilt_argument = raii_handle<prebuilt_argument_data>;

// builder, only received directly from Context, can grow indefinitely in size, not hashable
struct argument_builder
{
public:
    argument_builder& add(image const& img)
    {
        _resource_guids.push_back(img._resource.data.guid);

        auto& new_rv = _srvs.emplace_back();
        argument::populate_srv(new_rv, img);
        return *this;
    }
    argument_builder& add(buffer const& buffer)
    {
        _resource_guids.push_back(buffer._resource.data.guid);

        auto& new_rv = _srvs.emplace_back();
        new_rv.init_as_structured_buffer(buffer._resource.data.handle, buffer._info.size_bytes / buffer._info.stride_bytes, buffer._info.stride_bytes);
        return *this;
    }
    argument_builder& add(render_target const& rt)
    {
        _resource_guids.push_back(rt._resource.data.guid);

        auto& new_rv = _srvs.emplace_back();
        new_rv.init_as_tex2d(rt._resource.data.handle, rt._info.format, rt._info.num_samples > 1);
        return *this;
    }

    argument_builder& add_mutable(image const& img)
    {
        _resource_guids.push_back(img._resource.data.guid);

        auto& new_rv = _uavs.emplace_back();
        argument::populate_uav(new_rv, img);
        return *this;
    }
    argument_builder& add_mutable(buffer const& buffer)
    {
        _resource_guids.push_back(buffer._resource.data.guid);

        auto& new_rv = _uavs.emplace_back();
        new_rv.init_as_structured_buffer(buffer._resource.data.handle, buffer._info.size_bytes / buffer._info.stride_bytes, buffer._info.stride_bytes);
        return *this;
    }
    argument_builder& add_mutable(render_target const& rt)
    {
        _resource_guids.push_back(rt._resource.data.guid);

        auto& new_rv = _uavs.emplace_back();
        new_rv.init_as_tex2d(rt._resource.data.handle, rt._info.format, rt._info.num_samples > 1);
        return *this;
    }

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
    [[nodiscard]] prebuilt_argument make_graphics();
    [[nodiscard]] prebuilt_argument make_compute();

public:
    friend class Context;
    argument_builder(Context* parent) : _parent(parent)
    {
        _srvs.reserve(8);
        _uavs.reserve(8);
        _resource_guids.reserve(_srvs.size() + _uavs.size());
        _samplers.reserve(4);
    }
    Context* const _parent;

    cc::vector<phi::resource_view> _srvs;
    cc::vector<phi::resource_view> _uavs;
    cc::vector<uint64_t> _resource_guids;
    cc::vector<phi::sampler_config> _samplers;
};

}
