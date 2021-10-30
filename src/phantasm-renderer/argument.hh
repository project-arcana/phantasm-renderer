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

    resource_view_info& mips(uint32_t start, uint32_t size = uint32_t(-1))
    {
        rv.texture_info.mip_start = start;
        rv.texture_info.mip_size = size;
        return *this;
    }

    resource_view_info& tex_array(unsigned start, uint32_t size)
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
};

[[nodiscard]] inline resource_view_info resource_view_2d(texture const& tex, phi::format fmt, uint32_t mip_index = 0)
{
    resource_view_info res;
    res.rv.init_as_tex2d(tex.handle, fmt, false, mip_index);
    return res;
}

[[nodiscard]] inline resource_view_info resource_view_cube(texture const& tex, phi::format fmt)
{
    resource_view_info res;
    res.rv.init_as_texcube(tex.handle, fmt);
    return res;
}

[[nodiscard]] inline resource_view_info resource_view_structured_buffer(buffer const& buf, uint32_t num_elems, uint32_t stride_bytes, uint32_t first_element = 0)
{
    resource_view_info res;
    res.rv.init_as_structured_buffer(buf.handle, num_elems, stride_bytes, first_element);
    return res;
}

[[nodiscard]] inline resource_view_info resource_view_byte_address_buffer(buffer const& buf, uint32_t num_bytes, uint32_t offset_bytes = 0)
{
    resource_view_info res;
    res.rv.init_as_byte_address_buffer(buf.handle, num_bytes, offset_bytes);
    return res;
}

// an argument allows on-the-fly binding of arbitrary resources to a graphics- or compute pass
// it creates or looks up a cached phi::handle::shader_view when binding
struct PR_API argument
{
public:
    /// add a default-configured texture SRV
    void add(texture const& img, uint32_t mip_start = 0, uint32_t mip_size = uint32_t(-1))
    {
        phi::resource_view new_rv = {};
        new_rv.resource = img.handle;
        new_rv.dimension = phi::resource_view_dimension(e_incomplete_rv_dim_texture);
        new_rv.texture_info.mip_start = mip_start;
        new_rv.texture_info.mip_size = mip_size;
        _add_srv(new_rv);
    }

    /// add a default-configured structured buffer SRV
    void add(buffer const& buffer, uint32_t element_start = 0u)
    {
        phi::resource_view new_rv = {};
        new_rv.resource = buffer.handle;
        new_rv.dimension = phi::resource_view_dimension(e_incomplete_rv_dim_buffer);
        new_rv.buffer_info.element_start = element_start;
        _add_srv(new_rv);
    }

    /// add a raytracing acceleration structure
    void add(phi::handle::accel_struct as)
    {
        phi::resource_view new_rv = {};
        new_rv.init_as_accel_struct(as);
        _add_srv(new_rv);
    }

    /// add a configured SRV
    void add(resource_view_info const& rvi) { _add_srv(rvi.rv); }

    /// add a default-configured texture UAV
    void add_mutable(texture const& img, uint32_t mip_start = 0, uint32_t mip_size = uint32_t(-1))
    {
        phi::resource_view new_rv = {};
        new_rv.resource = img.handle;
        new_rv.dimension = phi::resource_view_dimension(e_incomplete_rv_dim_texture);
        new_rv.texture_info.mip_start = mip_start;
        new_rv.texture_info.mip_size = mip_size;
        _add_uav(new_rv);
    }

    /// add a default-configured buffer UAV
    void add_mutable(buffer const& buffer, uint32_t element_start = 0u)
    {
        phi::resource_view new_rv = {};
        new_rv.resource = buffer.handle;
        new_rv.dimension = phi::resource_view_dimension(e_incomplete_rv_dim_buffer);
        new_rv.buffer_info.element_start = element_start;
        _add_uav(new_rv);
    }

    /// add a configured UAV
    void add_mutable(resource_view_info const& rvi) { _add_uav(rvi.rv); }

    void add_sampler(pr::sampler_filter filter, uint32_t anisotropy = 16u, pr::sampler_address_mode address_mode = pr::sampler_address_mode::wrap)
    {
        add_sampler(pr::sampler_config(filter, anisotropy, address_mode));
    }

    void add_sampler(pr::sampler_config const& config)
    {
        CC_ASSERT_MSG(!_info.get().samplers.full(), "pr::argument samplers full\ncache-access arguments are fixed size,\n"
                                                    "use persistent prebuilt_arguments from Context::build_argument() instead");

        _info.get().samplers.push_back(config);
    }

    uint32_t get_num_srvs() const { return uint32_t(_info.get().srvs.size()); }
    uint32_t get_num_uavs() const { return uint32_t(_info.get().uavs.size()); }
    uint32_t get_num_samplers() const { return uint32_t(_info.get().samplers.size()); }

    void clear()
    {
        _info.get().srvs.clear();
        _info.get().uavs.clear();
        _info.get().samplers.clear();
    }

    static void fill_default_srv(pr::Context* ctx, phi::resource_view& new_rv, pr::texture const& img, uint32_t mip_start, uint32_t mip_size);

    static void fill_default_srv(pr::Context* ctx, phi::resource_view& new_rv, pr::buffer const& buf, uint32_t element_start = 0u);

    static void fill_default_uav(pr::Context* ctx, phi::resource_view& new_rv, pr::texture const& img, uint32_t mip_start, uint32_t mip_size);

    static void fill_default_uav(pr::Context* ctx, phi::resource_view& new_rv, pr::buffer const& buf, uint32_t element_start = 0u);

private:
    void _add_srv(phi::resource_view rv)
    {
        CC_ASSERT_MSG(!_info.get().srvs.full(), "pr::argument SRVs full\ncache-access arguments are fixed size,\n"
                                                "use persistent prebuilt_arguments from Context::build_argument() instead");

        _info.get().srvs.push_back(rv);
    }

    void _add_uav(phi::resource_view rv)
    {
        CC_ASSERT_MSG(!_info.get().uavs.full(), "pr::argument UAVs full\ncache-access arguments are fixed size,\n"
                                                "use persistent prebuilt_arguments from Context::build_argument() instead");

        _info.get().uavs.push_back(rv);
    }

    // fully automated resource view creation require extended information about the given resource
    // since that is not available at creation time, we store them as "incomplete" resource views initially
    // which are then fixed-up before hashing and cache-lookup
    void _fixup_incomplete_resource_views(pr::Context* ctx);

    enum e_incomplete_rv_dimensions : uint8_t
    {
        e_incomplete_rv_dim_texture = uint8_t(phi::resource_view_dimension::MAX_DIMENSION_RANGE) + 1,
        e_incomplete_rv_dim_buffer = uint8_t(phi::resource_view_dimension::MAX_DIMENSION_RANGE) + 2,
    };

private:
    friend class raii::Frame;
    friend class Context;
    hashable_storage<shader_view_info> _info;
};

[[nodiscard]] inline resource_view_info resource_view_buffer(buffer const& buf, Context& ctx, uint32_t element_start = 0u)
{
    resource_view_info res;
    argument::fill_default_srv(&ctx, res.rv, buf, element_start);
    return res;
}

[[nodiscard]] inline resource_view_info resource_view_texture(texture const& tex, Context& ctx, uint32_t mip_start = 0u, uint32_t mip_size = 1u)
{
    resource_view_info res;
    argument::fill_default_srv(&ctx, res.rv, tex, mip_start, mip_size);
    return res;
}

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

    argument_builder& add(texture const& img, uint32_t mip_start = 0, uint32_t mip_size = uint32_t(-1))
    {
        auto& new_rv = _srvs.emplace_back();
        argument::fill_default_srv(_parent, new_rv, img, mip_start, mip_size);
        return *this;
    }
    argument_builder& add(buffer const& buffer, uint32_t element_start = 0)
    {
        auto& new_rv = _srvs.emplace_back();
        pr::argument::fill_default_srv(_parent, new_rv, buffer, element_start);
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

    argument_builder& add_mutable(texture const& img, uint32_t mip_start = 0, uint32_t mip_size = uint32_t(-1))
    {
        auto& new_rv = _uavs.emplace_back();
        argument::fill_default_uav(_parent, new_rv, img, mip_start, mip_size);
        return *this;
    }

    argument_builder& add_mutable(buffer const& buffer, uint32_t element_start = 0)
    {
        auto& new_rv = _uavs.emplace_back();
        pr::argument::fill_default_uav(_parent, new_rv, buffer, element_start);
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
