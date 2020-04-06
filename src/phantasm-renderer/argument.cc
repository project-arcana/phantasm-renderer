#include "argument.hh"

#include <phantasm-hardware-interface/Backend.hh>

#include "Context.hh"

void pr::prebuilt_argument_data::destroy(pr::Context* ctx) { ctx->freeShaderView(_sv); }

void pr::argument::add(const pr::image& img)
{
    _info.get().srv_guids.push_back(img._resource.data.guid);

    auto& new_rv = _info.get().srvs.emplace_back();
    populate_srv(new_rv, img);
}

void pr::argument::add(const pr::buffer& buffer)
{
    _info.get().srv_guids.push_back(buffer._resource.data.guid);

    auto& new_rv = _info.get().srvs.emplace_back();
    new_rv.init_as_structured_buffer(buffer._resource.data.handle, buffer._info.size_bytes / buffer._info.stride_bytes, buffer._info.stride_bytes);
}

void pr::argument::add(const pr::render_target& rt)
{
    _info.get().srv_guids.push_back(rt._resource.data.guid);

    auto& new_rv = _info.get().srvs.emplace_back();
    new_rv.init_as_tex2d(rt._resource.data.handle, rt._info.format, rt._info.num_samples > 1);
}

void pr::argument::add_mutable(const pr::image& img)
{
    _info.get().uav_guids.push_back(img._resource.data.guid);

    auto& new_rv = _info.get().uavs.emplace_back();
    populate_uav(new_rv, img);
}

void pr::argument::add_mutable(const pr::buffer& buffer)
{
    _info.get().uav_guids.push_back(buffer._resource.data.guid);

    auto& new_rv = _info.get().uavs.emplace_back();
    new_rv.init_as_structured_buffer(buffer._resource.data.handle, buffer._info.size_bytes / buffer._info.stride_bytes, buffer._info.stride_bytes);
}

void pr::argument::add_mutable(const pr::render_target& rt)
{
    _info.get().uav_guids.push_back(rt._resource.data.guid);

    auto& new_rv = _info.get().uavs.emplace_back();
    new_rv.init_as_tex2d(rt._resource.data.handle, rt._info.format, rt._info.num_samples > 1);
}

void pr::argument::populate_srv(phi::resource_view& new_rv, const pr::image& img)
{
    new_rv.resource = img._resource.data.handle;

    if (img._info.dim == phi::texture_dimension::t1d)
    {
        new_rv.dimension = phi::resource_view_dimension::texture1d;
        new_rv.texture_info.array_size = 1;
        new_rv.texture_info.array_start = 0;
    }
    else if (img._info.dim == phi::texture_dimension::t2d)
    {
        new_rv.texture_info.array_size = img._info.depth_or_array_size;
        new_rv.texture_info.array_start = 0;
        new_rv.dimension = img._info.depth_or_array_size > 1 ? phi::resource_view_dimension::texture2d_array : phi::resource_view_dimension::texture2d;

        if (img._info.depth_or_array_size == 6)
            new_rv.dimension = phi::resource_view_dimension::texturecube;
    }
    else
    {
        new_rv.texture_info.array_size = img._info.depth_or_array_size;
        new_rv.texture_info.array_start = 0;
        new_rv.dimension = phi::resource_view_dimension::texture3d;
    }

    new_rv.texture_info.mip_start = 0;
    new_rv.texture_info.mip_size = img._info.num_mips == 0 ? unsigned(-1) : img._info.num_mips;
}

void pr::argument::populate_uav(phi::resource_view& new_rv, const pr::image& img)
{
    new_rv.resource = img._resource.data.handle;

    if (img._info.dim == phi::texture_dimension::t1d)
    {
        new_rv.dimension = phi::resource_view_dimension::texture1d;
        new_rv.texture_info.array_size = 1;
        new_rv.texture_info.array_start = 0;
    }
    else if (img._info.dim == phi::texture_dimension::t2d)
    {
        new_rv.texture_info.array_size = img._info.depth_or_array_size;
        new_rv.texture_info.array_start = 0;
        new_rv.dimension = img._info.depth_or_array_size > 1 ? phi::resource_view_dimension::texture2d_array : phi::resource_view_dimension::texture2d;

        if (img._info.depth_or_array_size == 6)
            new_rv.dimension = phi::resource_view_dimension::texturecube;
    }
    else
    {
        new_rv.texture_info.array_size = img._info.depth_or_array_size;
        new_rv.texture_info.array_start = 0;
        new_rv.dimension = phi::resource_view_dimension::texture3d;
    }

    new_rv.texture_info.mip_start = 0;
    new_rv.texture_info.mip_size = img._info.num_mips == 0 ? unsigned(-1) : img._info.num_mips;
}

pr::prebuilt_argument pr::argument_builder::make_graphics()
{
    return {prebuilt_argument_data{_parent->get_backend().createShaderView(_srvs, _uavs, _samplers, false), phi::handle::null_resource, 0}, _parent};
}

pr::prebuilt_argument pr::argument_builder::make_compute()
{
    return {prebuilt_argument_data{_parent->get_backend().createShaderView(_srvs, _uavs, _samplers, true), phi::handle::null_resource, 0}, _parent};
}
