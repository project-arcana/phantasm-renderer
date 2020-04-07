#include "argument.hh"

#include <phantasm-hardware-interface/Backend.hh>

#include "Context.hh"

void pr::argument::add(const texture &img)
{
    _info.get().srv_guids.push_back(img.res.guid);

    auto& new_rv = _info.get().srvs.emplace_back();
    populate_srv(new_rv, img);
}

void pr::argument::add(const pr::buffer& buffer)
{
    _info.get().srv_guids.push_back(buffer.res.guid);

    auto& new_rv = _info.get().srvs.emplace_back();
    new_rv.init_as_structured_buffer(buffer.res.handle, buffer.info.size_bytes / buffer.info.stride_bytes, buffer.info.stride_bytes);
}

void pr::argument::add(const pr::render_target& rt)
{
    _info.get().srv_guids.push_back(rt.res.guid);

    auto& new_rv = _info.get().srvs.emplace_back();
    new_rv.init_as_tex2d(rt.res.handle, rt.info.format, rt.info.num_samples > 1);
}

void pr::argument::add_mutable(const pr::texture& img)
{
    _info.get().uav_guids.push_back(img.res.guid);

    auto& new_rv = _info.get().uavs.emplace_back();
    populate_uav(new_rv, img);
}

void pr::argument::add_mutable(const pr::buffer& buffer)
{
    _info.get().uav_guids.push_back(buffer.res.guid);

    auto& new_rv = _info.get().uavs.emplace_back();
    new_rv.init_as_structured_buffer(buffer.res.handle, buffer.info.size_bytes / buffer.info.stride_bytes, buffer.info.stride_bytes);
}

void pr::argument::add_mutable(const pr::render_target& rt)
{
    _info.get().uav_guids.push_back(rt.res.guid);

    auto& new_rv = _info.get().uavs.emplace_back();
    new_rv.init_as_tex2d(rt.res.handle, rt.info.format, rt.info.num_samples > 1);
}

void pr::argument::populate_srv(phi::resource_view& new_rv, const pr::texture& img)
{
    new_rv.resource = img.res.handle;

    if (img.info.dim == phi::texture_dimension::t1d)
    {
        new_rv.dimension = phi::resource_view_dimension::texture1d;
        new_rv.texture_info.array_size = 1;
        new_rv.texture_info.array_start = 0;
    }
    else if (img.info.dim == phi::texture_dimension::t2d)
    {
        new_rv.texture_info.array_size = img.info.depth_or_array_size;
        new_rv.texture_info.array_start = 0;
        new_rv.dimension = img.info.depth_or_array_size > 1 ? phi::resource_view_dimension::texture2d_array : phi::resource_view_dimension::texture2d;

        if (img.info.depth_or_array_size == 6)
            new_rv.dimension = phi::resource_view_dimension::texturecube;
    }
    else
    {
        new_rv.texture_info.array_size = img.info.depth_or_array_size;
        new_rv.texture_info.array_start = 0;
        new_rv.dimension = phi::resource_view_dimension::texture3d;
    }

    new_rv.texture_info.mip_start = 0;
    new_rv.texture_info.mip_size = img.info.num_mips == 0 ? unsigned(-1) : img.info.num_mips;
}

void pr::argument::populate_uav(phi::resource_view& new_rv, const pr::texture& img)
{
    new_rv.resource = img.res.handle;

    if (img.info.dim == phi::texture_dimension::t1d)
    {
        new_rv.dimension = phi::resource_view_dimension::texture1d;
        new_rv.texture_info.array_size = 1;
        new_rv.texture_info.array_start = 0;
    }
    else if (img.info.dim == phi::texture_dimension::t2d)
    {
        new_rv.texture_info.array_size = img.info.depth_or_array_size;
        new_rv.texture_info.array_start = 0;
        new_rv.dimension = img.info.depth_or_array_size > 1 ? phi::resource_view_dimension::texture2d_array : phi::resource_view_dimension::texture2d;

        if (img.info.depth_or_array_size == 6)
            new_rv.dimension = phi::resource_view_dimension::texturecube;
    }
    else
    {
        new_rv.texture_info.array_size = img.info.depth_or_array_size;
        new_rv.texture_info.array_start = 0;
        new_rv.dimension = phi::resource_view_dimension::texture3d;
    }

    new_rv.texture_info.mip_start = 0;
    new_rv.texture_info.mip_size = img.info.num_mips == 0 ? unsigned(-1) : img.info.num_mips;
}

pr::auto_prebuilt_argument pr::argument_builder::make_graphics()
{
    return {prebuilt_argument{_parent->get_backend().createShaderView(_srvs, _uavs, _samplers, false), phi::handle::null_resource, 0}, _parent};
}

pr::auto_prebuilt_argument pr::argument_builder::make_compute()
{
    return {prebuilt_argument{_parent->get_backend().createShaderView(_srvs, _uavs, _samplers, true), phi::handle::null_resource, 0}, _parent};
}
