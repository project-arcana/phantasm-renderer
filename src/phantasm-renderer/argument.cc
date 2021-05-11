#include "argument.hh"

#include <phantasm-hardware-interface/Backend.hh>

#include "Context.hh"

void pr::argument::fill_default_srv(pr::Context* ctx, phi::resource_view& new_rv, const pr::texture& img, uint32_t mip_start, uint32_t mip_size)
{
    auto const& texDesc = ctx->get_backend().getResourceTextureDescription(img.handle);

    new_rv.resource = img.handle;
    new_rv.texture_info.pixel_format = texDesc.fmt;
    new_rv.texture_info.mip_start = mip_start;

    if (mip_size != unsigned(-1))
    {
        new_rv.texture_info.mip_size = mip_size;
    }
    else
    {
        // automatic, but attempt to derive from info
        new_rv.texture_info.mip_size = texDesc.num_mips == 0 ? unsigned(-1) : texDesc.num_mips;
    }

    if (texDesc.dim == phi::texture_dimension::t1d)
    {
        new_rv.dimension = phi::resource_view_dimension::texture1d;

        new_rv.texture_info.array_size = 1;
        new_rv.texture_info.array_start = 0;
    }
    else if (texDesc.dim == phi::texture_dimension::t2d)
    {
        if (texDesc.depth_or_array_size == 6)
            new_rv.dimension = phi::resource_view_dimension::texturecube;
        else if (texDesc.num_samples > 1)
            new_rv.dimension = texDesc.depth_or_array_size > 1 ? phi::resource_view_dimension::texture2d_ms_array : phi::resource_view_dimension::texture2d_ms;
        else
            new_rv.dimension = texDesc.depth_or_array_size > 1 ? phi::resource_view_dimension::texture2d_array : phi::resource_view_dimension::texture2d;

        new_rv.texture_info.array_size = texDesc.depth_or_array_size;
        new_rv.texture_info.array_start = 0;
    }
    else /* (== phi::texture_dimension::t3d) */
    {
        new_rv.dimension = phi::resource_view_dimension::texture3d;
        new_rv.texture_info.array_size = texDesc.depth_or_array_size;
        new_rv.texture_info.array_start = 0;
    }
}

void pr::argument::fill_default_srv(pr::Context* ctx, phi::resource_view& new_rv, pr::buffer const& buf, uint32_t element_start)
{
    auto const& bufDesc = ctx->get_backend().getResourceBufferDescription(buf.handle);

    CC_ASSERT(bufDesc.stride_bytes > 0 && "buffer used as SRV has no stride, pass a stride during creation");
    uint32_t const num_elems = bufDesc.size_bytes / bufDesc.stride_bytes;
    CC_ASSERT(element_start < num_elems && "element_start is OOB");
    new_rv.init_as_structured_buffer(buf.handle, num_elems - element_start, bufDesc.stride_bytes, element_start);
}

void pr::argument::fill_default_uav(pr::Context* ctx, phi::resource_view& new_rv, const pr::texture& img, uint32_t mip_start, uint32_t mip_size)
{
    // no difference in handling right now
    fill_default_srv(ctx, new_rv, img, mip_start, mip_size);
}

void pr::argument::fill_default_uav(pr::Context* ctx, phi::resource_view& new_rv, pr::buffer const& buf, uint32_t element_start)
{
    // no difference in handling right now
    fill_default_srv(ctx, new_rv, buf, element_start);
}

void pr::argument::_fixup_incomplete_resource_views(pr::Context* ctx)
{
    for (auto& srv : this->_info.get().srvs)
    {
        if (uint8_t(srv.dimension) == e_incomplete_rv_dim_texture)
        {
            auto const tex = pr::texture{{srv.resource}};
            fill_default_srv(ctx, srv, tex, srv.texture_info.mip_start, srv.texture_info.mip_size);
        }
        else if (uint8_t(srv.dimension) == e_incomplete_rv_dim_buffer)
        {
            auto const buf = pr::buffer{{srv.resource}};
            fill_default_srv(ctx, srv, buf, srv.buffer_info.element_start);
        }
    }

    for (auto& uav : this->_info.get().uavs)
    {
        if (uint8_t(uav.dimension) == e_incomplete_rv_dim_texture)
        {
            auto const tex = pr::texture{{uav.resource}};
            fill_default_srv(ctx, uav, tex, uav.texture_info.mip_start, uav.texture_info.mip_size);
        }
        else if (uint8_t(uav.dimension) == e_incomplete_rv_dim_buffer)
        {
            auto const buf = pr::buffer{{uav.resource}};
            fill_default_srv(ctx, uav, buf, uav.buffer_info.element_start);
        }
    }
}

pr::auto_prebuilt_argument pr::argument_builder::make_graphics()
{
    return {prebuilt_argument{_parent->get_backend().createShaderView(_srvs, _uavs, _samplers, false)}, _parent};
}

pr::auto_prebuilt_argument pr::argument_builder::make_compute()
{
    return {prebuilt_argument{_parent->get_backend().createShaderView(_srvs, _uavs, _samplers, true)}, _parent};
}
