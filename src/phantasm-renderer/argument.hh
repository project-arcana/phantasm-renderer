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
enum e_incomplete_rv_dimensions : uint32_t
{
    e_incomplete_rv_dim_texture = uint32_t(phi::resource_view_dimension::MAX_DIMENSION_RANGE) + 1,
    e_incomplete_rv_dim_buffer = uint32_t(phi::resource_view_dimension::MAX_DIMENSION_RANGE) + 2,
};

struct PR_API view : phi::resource_view
{
    constexpr view() = default;
    constexpr view(phi::resource_view const& rv) : phi::resource_view(rv) {}

    // default texture view
    constexpr view(buffer buf, uint32_t element_start = 0)
    {
        resource = buf.handle;
        dimension = phi::resource_view_dimension(e_incomplete_rv_dim_buffer);
        buffer_info.element_start = element_start;
    }

    // default buffer view
    constexpr view(texture tex, uint32_t mip_start = 0, uint32_t mip_size = uint32_t(-1))
    {
        resource = tex.handle;
        dimension = phi::resource_view_dimension(e_incomplete_rv_dim_texture);
        texture_info.mip_start = mip_start;
        texture_info.mip_size = mip_size;
    }
};

// an argument allows on-the-fly binding of arbitrary resources to a graphics- or compute pass
// it creates or looks up a cached phi::handle::shader_view when binding
struct PR_API argument
{
    // add a shader resource view (SRV)
    void add(view const& v) { _add_srv(v); }

    // add a default-configured texture SRV
    void add(texture img, uint32_t mip_start = 0, uint32_t mip_size = uint32_t(-1)) { _add_srv(view(img, mip_start, mip_size)); }

    // add a default-configured structured buffer SRV
    void add(buffer buffer, uint32_t element_start = 0u) { _add_srv(view(buffer, element_start)); }

    // add a raytracing acceleration structure (SRV)
    void add(phi::handle::accel_struct as) { _add_srv(view::accel_struct(as)); }

    // add an unordered access view (UAV)
    void add_mutable(view const& v) { _add_uav(v); }

    // add a default-configured texture UAV
    void add_mutable(texture img, uint32_t mip_start = 0, uint32_t mip_size = uint32_t(-1)) { _add_uav(view(img, mip_start, mip_size)); }

    // add a default-configured structured buffer UAV
    void add_mutable(buffer buffer, uint32_t element_start = 0u) { _add_uav(view(buffer, element_start)); }

    void add_sampler(pr::sampler_filter filter, uint32_t anisotropy = 16u, pr::sampler_address_mode address_mode = pr::sampler_address_mode::wrap)
    {
        add_sampler(pr::sampler_config(filter, anisotropy, address_mode));
    }

    void add_sampler(pr::sampler_config const& config)
    {
        CC_ASSERT_MSG(!samplers.full(), "pr::argument samplers full\ncache-access arguments are fixed size,\n"
                                        "use persistent prebuilt_arguments from Context::build_argument() instead");
        samplers.push_back(config);
    }

    uint32_t get_num_srvs() const { return uint32_t(srvs.size()); }
    uint32_t get_num_uavs() const { return uint32_t(uavs.size()); }
    uint32_t get_num_samplers() const { return uint32_t(samplers.size()); }

    void clear()
    {
        srvs.clear();
        uavs.clear();
        samplers.clear();
    }

    static void fill_default_srv(pr::Context* ctx, phi::resource_view& new_rv, pr::texture const& img, uint32_t mip_start, uint32_t mip_size);

    static void fill_default_srv(pr::Context* ctx, phi::resource_view& new_rv, pr::buffer const& buf, uint32_t element_start = 0u);

    static void fill_default_uav(pr::Context* ctx, phi::resource_view& new_rv, pr::texture const& img, uint32_t mip_start, uint32_t mip_size);

    static void fill_default_uav(pr::Context* ctx, phi::resource_view& new_rv, pr::buffer const& buf, uint32_t element_start = 0u);

private:
    void _add_srv(phi::resource_view rv)
    {
        CC_ASSERT_MSG(!srvs.full(), "pr::argument SRVs full\ncache-access arguments are fixed size,\n"
                                    "use persistent prebuilt_arguments from Context::build_argument() instead");

        srvs.push_back(rv);
    }

    void _add_uav(phi::resource_view rv)
    {
        CC_ASSERT_MSG(!uavs.full(), "pr::argument UAVs full\ncache-access arguments are fixed size,\n"
                                    "use persistent prebuilt_arguments from Context::build_argument() instead");

        uavs.push_back(rv);
    }

public:
    phi::flat_vector<view, 5> srvs;
    phi::flat_vector<view, 5> uavs;
    phi::flat_vector<phi::sampler_config, 2> samplers;
};

// fully automated resource view creation require extended information about the given resource
// since that is not available at creation time, we store them as "incomplete" resource views initially
// which are then fixed-up before hashing and cache-lookup
void fixup_incomplete_views(pr::Context* pCtx, cc::span<pr::view> srvs, cc::span<pr::view> uavs);

struct prebuilt_argument
{
    phi::handle::shader_view _sv = phi::handle::null_shader_view;
};

using auto_prebuilt_argument = auto_destroyer<prebuilt_argument, auto_mode::guard>;

// builder, only received directly from Context, can grow indefinitely in size, not hashable
struct PR_API argument_builder
{
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
