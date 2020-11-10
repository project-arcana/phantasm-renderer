#include "argument.hh"

#include <phantasm-hardware-interface/Backend.hh>

#include "Context.hh"

void pr::argument::fill_default_srv(phi::resource_view& new_rv, const pr::texture& img, unsigned mip_start, unsigned mip_size)
{
    new_rv.resource = img.res.handle;
    new_rv.texture_info.pixel_format = img.info.fmt;
    new_rv.texture_info.mip_start = mip_start;

    if (mip_size != unsigned(-1))
    {
        new_rv.texture_info.mip_size = mip_size;
    }
    else
    {
        // automatic, but attempt to derive from info
        new_rv.texture_info.mip_size = img.info.num_mips == 0 ? unsigned(-1) : img.info.num_mips;
    }

    if (img.info.dim == phi::texture_dimension::t1d)
    {
        new_rv.dimension = phi::resource_view_dimension::texture1d;

        new_rv.texture_info.array_size = 1;
        new_rv.texture_info.array_start = 0;
    }
    else if (img.info.dim == phi::texture_dimension::t2d)
    {
        if (img.info.depth_or_array_size == 6)
            new_rv.dimension = phi::resource_view_dimension::texturecube;
        else
            new_rv.dimension = img.info.depth_or_array_size > 1 ? phi::resource_view_dimension::texture2d_array : phi::resource_view_dimension::texture2d;

        new_rv.texture_info.array_size = img.info.depth_or_array_size;
        new_rv.texture_info.array_start = 0;
    }
    else /* (== phi::texture_dimension::t3d) */
    {
        new_rv.dimension = phi::resource_view_dimension::texture3d;
        new_rv.texture_info.array_size = img.info.depth_or_array_size;
        new_rv.texture_info.array_start = 0;
    }
}

void pr::argument::fill_default_uav(phi::resource_view& new_rv, const pr::texture& img, unsigned mip_start, unsigned mip_size)
{
    // no difference in handling right now
    fill_default_srv(new_rv, img, mip_start, mip_size);
}

pr::auto_prebuilt_argument pr::argument_builder::make_graphics()
{
    return {prebuilt_argument{_parent->get_backend().createShaderView(_srvs, _uavs, _samplers, false)}, _parent};
}

pr::auto_prebuilt_argument pr::argument_builder::make_compute()
{
    return {prebuilt_argument{_parent->get_backend().createShaderView(_srvs, _uavs, _samplers, true)}, _parent};
}
