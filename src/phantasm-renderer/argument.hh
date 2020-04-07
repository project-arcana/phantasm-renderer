#pragma once

#include <clean-core/vector.hh>

#include <phantasm-hardware-interface/arguments.hh>

#include <phantasm-renderer/common/hashable_storage.hh>
#include <phantasm-renderer/common/state_info.hh>

#include <phantasm-renderer/format.hh>
#include <phantasm-renderer/fwd.hh>
#include <phantasm-renderer/resource_types.hh>

namespace pr
{
class Context;

struct resource_view_info
{
    resource_view_info& format(pr::format fmt)
    {
        rv.pixel_format = fmt;
        return *this;
    }

    resource_view_info& mips(unsigned start, unsigned size = unsigned(-1))
    {
        rv.texture_info.mip_start = start;
        rv.texture_info.mip_size = size;
        return *this;
    }

    resource_view_info& tex_array(unsigned start, unsigned size)
    {
        if (size > 1)
        {
            if (rv.dimension == phi::resource_view_dimension::texture2d)
                rv.dimension = phi::resource_view_dimension::texture2d_array;
            else if (rv.dimension == phi::resource_view_dimension::texture2d_ms)
                rv.dimension = phi::resource_view_dimension::texture2d_ms_array;
            else if (rv.dimension == phi::resource_view_dimension::texture1d)
                rv.dimension = phi::resource_view_dimension::texture1d_array;
        }

        rv.texture_info.array_start = start;
        rv.texture_info.array_size = size;
        return *this;
    }

    resource_view_info& dimension(phi::resource_view_dimension dim)
    {
        rv.dimension = dim;
        return *this;
    }

    phi::resource_view rv;
    uint64_t guid;
};

[[nodiscard]] inline resource_view_info resource_view_2d(texture const& tex, unsigned mip_start = 0, unsigned mip_size = unsigned(-1))
{
    resource_view_info res;
    res.guid = tex.res.guid;
    res.rv.init_as_tex2d(tex.res.handle, tex.info.fmt, false, mip_start, mip_size);
    return res;
}

[[nodiscard]] inline resource_view_info resource_view_cube(texture const& tex)
{
    resource_view_info res;
    res.guid = tex.res.guid;
    res.rv.init_as_texcube(tex.res.handle, tex.info.fmt);
    return res;
}

// fixed size, hashable, no raw resources allowed
struct argument
{
public:
    /// add a default-configured texture SRV
    void add(texture const& img);
    /// add a default-configured buffer SRV
    void add(buffer const& buffer);
    /// add a default-configured 2D texture SRV
    void add(render_target const& rt);

    /// add a configured SRV
    void add(resource_view_info const& rvi)
    {
        _info.get().srvs.push_back(rvi.rv);
        _info.get().srv_guids.push_back(rvi.guid);
    }

    /// add a default-configured texture UAV
    void add_mutable(texture const& img);
    /// add a default-configured buffer UAV
    void add_mutable(buffer const& buffer);
    /// add a default-configured 2D texture UAV
    void add_mutable(render_target const& rt);

    /// add a configured UAV
    void add_mutable(resource_view_info const& rvi)
    {
        _info.get().uavs.push_back(rvi.rv);
        _info.get().uav_guids.push_back(rvi.guid);
    }

    void add_sampler(phi::sampler_filter filter, unsigned anisotropy = 16u)
    {
        auto& new_sampler = _info.get().samplers.emplace_back();
        new_sampler.init_default(filter, anisotropy);
    }

    void add_sampler(phi::sampler_config const& config) { _info.get().samplers.push_back(config); }

private:
    friend struct argument_builder;
    static void populate_srv(phi::resource_view& new_rv, pr::texture const& img, unsigned mip_start, unsigned mip_size);
    static void populate_uav(phi::resource_view& new_rv, pr::texture const& img, unsigned mip_start, unsigned mip_size);

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
    //
    // add SRVs

    argument_builder& add(texture const& img)
    {
        auto& new_rv = _srvs.emplace_back();
        argument::populate_srv(new_rv, img, 0, unsigned(-1));
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
    argument_builder& add(resource_view_info const& rvi)
    {
        _srvs.push_back(rvi.rv);
        return *this;
    }
    argument_builder& add(phi::resource_view const& raw_rv)
    {
        _srvs.push_back(raw_rv);
        return *this;
    }

    //
    // add UAVs

    argument_builder& add_mutable(texture const& img)
    {
        auto& new_rv = _uavs.emplace_back();
        argument::populate_uav(new_rv, img, 0, unsigned(-1));
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
    argument_builder& add_mutable(resource_view_info const& rvi)
    {
        _uavs.push_back(rvi.rv);
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
