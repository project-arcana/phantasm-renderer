#pragma once

#include <clean-core/alloc_vector.hh>

#include <phantasm-hardware-interface/arguments.hh>

#include <phantasm-renderer/common/api.hh>
#include <phantasm-renderer/common/hashable_storage.hh>
#include <phantasm-renderer/common/state_info.hh>

#include <phantasm-renderer/enums.hh>
#include <phantasm-renderer/fwd.hh>
#include <phantasm-renderer/resource_types.hh>

namespace pr
{
struct PR_API resource_view_info
{
    resource_view_info& format(pr::format fmt)
    {
        rv.texture_info.pixel_format = fmt;
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

[[nodiscard]] inline resource_view_info resource_view_2d(render_target const& tex, unsigned mip_start = 0, unsigned mip_size = unsigned(-1))
{
    resource_view_info res;
    res.guid = tex.res.guid;
    res.rv.init_as_tex2d(tex.res.handle, tex.info.format, false, mip_start, mip_size);
    return res;
}

[[nodiscard]] inline resource_view_info resource_view_cube(texture const& tex)
{
    resource_view_info res;
    res.guid = tex.res.guid;
    res.rv.init_as_texcube(tex.res.handle, tex.info.fmt);
    return res;
}

[[nodiscard]] inline resource_view_info resource_view_structured_buffer(buffer const& buf, unsigned num_elems, unsigned stride_bytes, unsigned first_element = 0)
{
    resource_view_info res;
    res.guid = buf.res.guid;
    res.rv.init_as_structured_buffer(buf.res.handle, num_elems, stride_bytes, first_element);
    return res;
}

// fixed size, hashable, no raw resources allowed
struct PR_API argument
{
public:
    /// add a default-configured texture SRV
    void add(texture const& img)
    {
        phi::resource_view new_rv;
        fill_default_srv(new_rv, img, 0, unsigned(-1));
        _add_srv(new_rv, img.res.guid);
    }

    /// add a default-configured structured buffer SRV
    void add(buffer const& buffer)
    {
        CC_ASSERT(buffer.info.stride_bytes > 0 && "buffer used as SRV has no stride, pass a stride during creation");
        _add_srv(phi::resource_view::structured_buffer(buffer.res.handle, buffer.info.size_bytes / buffer.info.stride_bytes, buffer.info.stride_bytes),
                 buffer.res.guid);
    }

    /// add a default-configured 2D texture SRV
    void add(render_target const& rt) { _add_srv(phi::resource_view::tex2d(rt.res.handle, rt.info.format, rt.info.num_samples > 1), rt.res.guid); }

    /// add a configured SRV
    void add(resource_view_info const& rvi) { _add_srv(rvi.rv, rvi.guid); }

    /// add a default-configured texture UAV
    void add_mutable(texture const& img)
    {
        phi::resource_view new_rv;
        fill_default_uav(new_rv, img, 0, unsigned(-1));
        _add_uav(new_rv, img.res.guid);
    }

    /// add a default-configured buffer UAV
    void add_mutable(buffer const& buffer)
    {
        CC_ASSERT(buffer.info.stride_bytes > 0 && "buffer used as UAV has no stride, pass a stride during creation");
        _add_uav(phi::resource_view::structured_buffer(buffer.res.handle, buffer.info.size_bytes / buffer.info.stride_bytes, buffer.info.stride_bytes),
                 buffer.res.guid);
    }

    /// add a default-configured 2D texture UAV
    void add_mutable(render_target const& rt)
    {
        _add_uav(phi::resource_view::tex2d(rt.res.handle, rt.info.format, rt.info.num_samples > 1), rt.res.guid);
    }

    /// add a configured UAV
    void add_mutable(resource_view_info const& rvi) { _add_uav(rvi.rv, rvi.guid); }

    void add_sampler(pr::sampler_filter filter, unsigned anisotropy = 16u, pr::sampler_address_mode address_mode = pr::sampler_address_mode::wrap)
    {
        add_sampler(pr::sampler_config(filter, anisotropy, address_mode));
    }

    void add_sampler(pr::sampler_config const& config)
    {
        CC_ASSERT_MSG(!_info.get().uavs.full(), "pr::argument samplers full\ncache-access arguments are fixed size,\n"
                                                "use persistent prebuilt_arguments from Context::build_argument() instead");

        _info.get().samplers.push_back(config);
    }

    unsigned get_num_srvs() const { return unsigned(_info.get().srvs.size()); }
    unsigned get_num_uavs() const { return unsigned(_info.get().uavs.size()); }
    unsigned get_num_samplers() const { return unsigned(_info.get().samplers.size()); }

private:
    void _add_srv(phi::resource_view rv, uint64_t guid)
    {
        CC_ASSERT_MSG(!_info.get().srvs.full(), "pr::argument SRVs full\ncache-access arguments are fixed size,\n"
                                                "use persistent prebuilt_arguments from Context::build_argument() instead");

        _info.get().srvs.push_back(rv);
        _info.get().srv_guids.push_back(guid);
    }

    void _add_uav(phi::resource_view rv, uint64_t guid)
    {
        CC_ASSERT_MSG(!_info.get().uavs.full(), "pr::argument UAVs full\ncache-access arguments are fixed size,\n"
                                                "use persistent prebuilt_arguments from Context::build_argument() instead");

        _info.get().uavs.push_back(rv);
        _info.get().uav_guids.push_back(guid);
    }

    friend struct argument_builder;
    static void fill_default_srv(phi::resource_view& new_rv, pr::texture const& img, unsigned mip_start, unsigned mip_size);
    static void fill_default_uav(phi::resource_view& new_rv, pr::texture const& img, unsigned mip_start, unsigned mip_size);

private:
    friend class raii::Frame;
    friend class Context;
    hashable_storage<shader_view_info> _info;
};

struct prebuilt_argument
{
    phi::handle::shader_view _sv = phi::handle::null_shader_view;
};

using auto_prebuilt_argument = auto_destroyer<prebuilt_argument, auto_mode::guard>;

// builder, only received directly from Context, can grow indefinitely in size, not hashable
struct PR_API argument_builder
{
public:
    //
    // add SRVs

    argument_builder& add(texture const& img)
    {
        auto& new_rv = _srvs.emplace_back();
        argument::fill_default_srv(new_rv, img, 0, unsigned(-1));
        return *this;
    }
    argument_builder& add(buffer const& buffer)
    {
        CC_ASSERT(buffer.info.stride_bytes > 0 && "buffer used as SRV argument has no stride, pass a stride during creation");
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
        argument::fill_default_uav(new_rv, img, 0, unsigned(-1));
        return *this;
    }
    argument_builder& add_mutable(buffer const& buffer)
    {
        CC_ASSERT(buffer.info.stride_bytes > 0 && "buffer used as UAV argument has no stride, pass a stride during creation");
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

    argument_builder& add_sampler(phi::sampler_filter filter, unsigned anisotropy = 16u, phi::sampler_address_mode address_mode = phi::sampler_address_mode::wrap)
    {
        auto& new_sampler = _samplers.emplace_back();
        new_sampler.init_default(filter, anisotropy, address_mode);
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
    argument_builder(Context* parent, cc::allocator* temp_alloc) : _parent(parent)
    {
        _srvs.reset_reserve(temp_alloc, 16);
        _uavs.reset_reserve(temp_alloc, 16);
        _samplers.reset_reserve(temp_alloc, 8);
    }
    Context* const _parent;

    cc::alloc_vector<phi::resource_view> _srvs;
    cc::alloc_vector<phi::resource_view> _uavs;
    cc::alloc_vector<phi::sampler_config> _samplers;
};

}
