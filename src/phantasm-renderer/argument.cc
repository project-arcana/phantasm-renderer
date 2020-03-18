#include "argument.hh"

#include "Context.hh"

void pr::shader_view::add_srv(const pr::image& img)
{
    _resource_guids.push_back(img._resource.data.guid);

    auto& new_rv = _srvs.emplace_back();
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

void pr::shader_view::add_srv(const pr::buffer& buffer)
{
    _resource_guids.push_back(buffer._resource.data.guid);

    auto& new_rv = _srvs.emplace_back();
    new_rv.init_as_structured_buffer(buffer._resource.data.handle, buffer._info.size_bytes / buffer._info.stride_bytes, buffer._info.stride_bytes);
}

void pr::shader_view::add_srv(const pr::render_target& rt)
{
    _resource_guids.push_back(rt._resource.data.guid);

    auto& new_rv = _srvs.emplace_back();
    new_rv.init_as_tex2d(rt._resource.data.handle, rt._info.format, rt._info.num_samples > 1);
}

void pr::shader_view::add_uav(const pr::image& img)
{
    _resource_guids.push_back(img._resource.data.guid);

    auto& new_rv = _uavs.emplace_back();
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

void pr::shader_view::add_uav(const pr::buffer& buffer)
{
    _resource_guids.push_back(buffer._resource.data.guid);

    auto& new_rv = _uavs.emplace_back();
    new_rv.init_as_structured_buffer(buffer._resource.data.handle, buffer._info.size_bytes / buffer._info.stride_bytes, buffer._info.stride_bytes);
}

void pr::shader_view::add_uav(const pr::render_target& rt)
{
    _resource_guids.push_back(rt._resource.data.guid);

    auto& new_rv = _uavs.emplace_back();
    new_rv.init_as_tex2d(rt._resource.data.handle, rt._info.format, rt._info.num_samples > 1);
}

void pr::baked_shader_view_data::destroy(pr::Context* ctx) { ctx->freeShaderView(_sv); }
