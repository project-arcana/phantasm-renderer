#include "util.hh"

#include <cstdarg>
#include <cstdio>

#include <phantasm-renderer/backend/d3d12/common/dxgi_format.hh>
#include <phantasm-renderer/backend/d3d12/common/native_enum.hh>

cc::capped_vector<D3D12_INPUT_ELEMENT_DESC, 16> pr::backend::d3d12::util::get_native_vertex_format(cc::span<const pr::backend::vertex_attribute_info> attrib_info)
{
    cc::capped_vector<D3D12_INPUT_ELEMENT_DESC, 16> res;
    for (auto const& ai : attrib_info)
    {
        D3D12_INPUT_ELEMENT_DESC& elem = res.emplace_back();
        elem.SemanticName = ai.semantic_name;
        elem.AlignedByteOffset = ai.offset;
        elem.Format = util::to_dxgi_format(ai.format);
        elem.SemanticIndex = 0;
        elem.InputSlot = 0;
        elem.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        elem.InstanceDataStepRate = 0;
    }

    return res;
}

void pr::backend::d3d12::util::set_object_name(ID3D12Object* object, const char* name, ...)
{
    if (name != nullptr)
    {
        char name_formatted[1024];
        {
            va_list args;
            va_start(args, name);
            ::vsprintf_s(name_formatted, 1024, name, args);
            va_end(args);
        }

        // Since recently, d3d12 object names can be set using non-wide strings
        // even though it doesn't look like it, this works perfectly with validation layers, PIX, Renderdoc, NSight and DRED
        object->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(name_formatted)), name_formatted);
    }
}

D3D12_SHADER_RESOURCE_VIEW_DESC pr::backend::d3d12::util::create_srv_desc(const pr::backend::shader_view_element& sve, ID3D12Resource* raw_resource)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv_desc.Format = util::to_dxgi_format(sve.pixel_format);
    srv_desc.ViewDimension = util::to_native_srv_dim(sve.dimension);

    using svd = shader_view_dimension;
    switch (sve.dimension)
    {
    case svd::buffer:
        srv_desc.Buffer.NumElements = sve.buffer_info.element_size;
        srv_desc.Buffer.FirstElement = sve.buffer_info.element_start;
        srv_desc.Buffer.StructureByteStride = sve.buffer_info.element_stride_bytes;
        break;

    case svd::raytracing_accel_struct:
        srv_desc.RaytracingAccelerationStructure.Location = raw_resource->GetGPUVirtualAddress();
        break;

    case svd::texture1d:
        srv_desc.Texture1D.MostDetailedMip = sve.texture_info.mip_start;
        srv_desc.Texture1D.MipLevels = sve.texture_info.mip_size;
        srv_desc.Texture1D.ResourceMinLODClamp = 0.f;
        break;

    case svd::texture1d_array:
        srv_desc.Texture1DArray.MostDetailedMip = sve.texture_info.mip_start;
        srv_desc.Texture1DArray.MipLevels = sve.texture_info.mip_size;
        srv_desc.Texture1DArray.ResourceMinLODClamp = 0.f;
        srv_desc.Texture1DArray.FirstArraySlice = sve.texture_info.array_start;
        srv_desc.Texture1DArray.ArraySize = sve.texture_info.array_size;
        break;

    case svd::texture2d:
        srv_desc.Texture2D.MostDetailedMip = sve.texture_info.mip_start;
        srv_desc.Texture2D.MipLevels = sve.texture_info.mip_size;
        srv_desc.Texture2D.ResourceMinLODClamp = 0.f;
        srv_desc.Texture2D.PlaneSlice = 0u;
        break;

    case svd::texture2d_array:
        srv_desc.Texture2DArray.MostDetailedMip = sve.texture_info.mip_start;
        srv_desc.Texture2DArray.MipLevels = sve.texture_info.mip_size;
        srv_desc.Texture2DArray.ResourceMinLODClamp = 0.f;
        srv_desc.Texture2DArray.PlaneSlice = 0u;
        srv_desc.Texture2DArray.FirstArraySlice = sve.texture_info.array_start;
        srv_desc.Texture2DArray.ArraySize = sve.texture_info.array_size;
        break;

    case svd::texture2d_ms:
        break;

    case svd::texture2d_ms_array:
        srv_desc.Texture2DMSArray.FirstArraySlice = sve.texture_info.array_start;
        srv_desc.Texture2DMSArray.ArraySize = sve.texture_info.array_size;
        break;

    case svd::texture3d:
        srv_desc.Texture3D.MostDetailedMip = sve.texture_info.mip_start;
        srv_desc.Texture3D.MipLevels = sve.texture_info.mip_size;
        srv_desc.Texture3D.ResourceMinLODClamp = 0.f;
        break;

    case svd::texturecube:
        srv_desc.TextureCube.MostDetailedMip = sve.texture_info.mip_start;
        srv_desc.TextureCube.MipLevels = sve.texture_info.mip_size;
        srv_desc.TextureCube.ResourceMinLODClamp = 0.f;
        break;

    case svd::texturecube_array:
        srv_desc.TextureCubeArray.MostDetailedMip = sve.texture_info.mip_start;
        srv_desc.TextureCubeArray.MipLevels = sve.texture_info.mip_size;
        srv_desc.TextureCubeArray.ResourceMinLODClamp = 0.f;
        srv_desc.TextureCubeArray.First2DArrayFace = sve.texture_info.array_start;
        srv_desc.TextureCubeArray.NumCubes = sve.texture_info.array_size;
        break;
    }

    return srv_desc;
}

D3D12_UNORDERED_ACCESS_VIEW_DESC pr::backend::d3d12::util::create_uav_desc(const pr::backend::shader_view_element& sve)
{
    D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
    uav_desc.Format = util::to_dxgi_format(sve.pixel_format);
    uav_desc.ViewDimension = util::to_native_uav_dim(sve.dimension);
    CC_ASSERT(uav_desc.ViewDimension != D3D12_UAV_DIMENSION_UNKNOWN && "Invalid UAV dimension");

    using svd = shader_view_dimension;
    switch (sve.dimension)
    {
    case svd::buffer:
        uav_desc.Buffer.NumElements = sve.buffer_info.element_size;
        uav_desc.Buffer.FirstElement = sve.buffer_info.element_start;
        uav_desc.Buffer.StructureByteStride = sve.buffer_info.element_stride_bytes;
        break;

    case svd::texture1d:
        uav_desc.Texture1D.MipSlice = sve.texture_info.mip_start;
        break;

    case svd::texture1d_array:
        uav_desc.Texture1DArray.MipSlice = sve.texture_info.mip_start;
        uav_desc.Texture1DArray.FirstArraySlice = sve.texture_info.array_start;
        uav_desc.Texture1DArray.ArraySize = sve.texture_info.array_size;
        break;

    case svd::texture2d:
        uav_desc.Texture2D.MipSlice = sve.texture_info.mip_start;
        uav_desc.Texture2D.PlaneSlice = 0u;
        break;

    case svd::texture2d_array:
        uav_desc.Texture2DArray.MipSlice = sve.texture_info.mip_start;
        uav_desc.Texture2DArray.PlaneSlice = 0u;
        uav_desc.Texture2DArray.FirstArraySlice = sve.texture_info.array_start;
        uav_desc.Texture2DArray.ArraySize = sve.texture_info.array_size;
        break;

    case svd::texture3d:
        uav_desc.Texture3D.FirstWSlice = sve.texture_info.array_start;
        uav_desc.Texture3D.WSize = sve.texture_info.array_size;
        uav_desc.Texture3D.MipSlice = sve.texture_info.mip_start;
        break;

    default:
        break;
    }

    return uav_desc;
}

D3D12_RENDER_TARGET_VIEW_DESC pr::backend::d3d12::util::create_rtv_desc(const pr::backend::shader_view_element& sve)
{
    D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {};
    rtv_desc.Format = util::to_dxgi_format(sve.pixel_format);
    rtv_desc.ViewDimension = util::to_native_rtv_dim(sve.dimension);

    using svd = shader_view_dimension;
    switch (sve.dimension)
    {
    case svd::buffer:
        rtv_desc.Buffer.NumElements = sve.buffer_info.element_size;
        rtv_desc.Buffer.FirstElement = sve.buffer_info.element_start;
        break;

    case svd::texture1d:
        rtv_desc.Texture1D.MipSlice = sve.texture_info.mip_start;
        break;

    case svd::texture1d_array:
        rtv_desc.Texture1DArray.MipSlice = sve.texture_info.mip_start;
        rtv_desc.Texture1DArray.FirstArraySlice = sve.texture_info.array_start;
        rtv_desc.Texture1DArray.ArraySize = sve.texture_info.array_size;
        break;

    case svd::texture2d:
        rtv_desc.Texture2D.MipSlice = sve.texture_info.mip_start;
        rtv_desc.Texture2D.PlaneSlice = 0u;
        break;

    case svd::texture2d_array:
        rtv_desc.Texture2DArray.MipSlice = sve.texture_info.mip_start;
        rtv_desc.Texture2DArray.PlaneSlice = 0u;
        rtv_desc.Texture2DArray.FirstArraySlice = sve.texture_info.array_start;
        rtv_desc.Texture2DArray.ArraySize = sve.texture_info.array_size;
        break;

    case svd::texture2d_ms_array:
        rtv_desc.Texture2DMSArray.FirstArraySlice = sve.texture_info.array_start;
        rtv_desc.Texture2DMSArray.ArraySize = sve.texture_info.array_size;
        break;

    case svd::texture3d:
        rtv_desc.Texture3D.MipSlice = sve.texture_info.mip_start;
        rtv_desc.Texture3D.FirstWSlice = sve.texture_info.array_start;
        rtv_desc.Texture3D.WSize = sve.texture_info.array_size;
        break;

    default:
        break;
    }

    return rtv_desc;
}

D3D12_DEPTH_STENCIL_VIEW_DESC pr::backend::d3d12::util::create_dsv_desc(const pr::backend::shader_view_element& sve)
{
    D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc = {};
    dsv_desc.Format = util::to_dxgi_format(sve.pixel_format);
    dsv_desc.ViewDimension = util::to_native_dsv_dim(sve.dimension);

    using svd = shader_view_dimension;
    switch (sve.dimension)
    {
    case svd::texture1d:
        dsv_desc.Texture1D.MipSlice = sve.texture_info.mip_start;
        break;

    case svd::texture1d_array:
        dsv_desc.Texture1DArray.MipSlice = sve.texture_info.mip_start;
        dsv_desc.Texture1DArray.FirstArraySlice = sve.texture_info.array_start;
        dsv_desc.Texture1DArray.ArraySize = sve.texture_info.array_size;
        break;

    case svd::texture2d:
        dsv_desc.Texture2D.MipSlice = sve.texture_info.mip_start;
        break;

    case svd::texture2d_array:
        dsv_desc.Texture2DArray.MipSlice = sve.texture_info.mip_start;
        dsv_desc.Texture2DArray.FirstArraySlice = sve.texture_info.array_start;
        dsv_desc.Texture2DArray.ArraySize = sve.texture_info.array_size;
        break;

    case svd::texture2d_ms_array:
        dsv_desc.Texture2DMSArray.FirstArraySlice = sve.texture_info.array_start;
        dsv_desc.Texture2DMSArray.ArraySize = sve.texture_info.array_size;
        break;

    default:
        break;
    }

    return dsv_desc;
}

D3D12_SAMPLER_DESC pr::backend::d3d12::util::create_sampler_desc(const pr::backend::sampler_config& config)
{
    D3D12_SAMPLER_DESC sampler_desc = {};
    sampler_desc.Filter = util::to_native(config.filter, config.compare_func != sampler_compare_func::disabled);
    sampler_desc.AddressU = util::to_native(config.address_u);
    sampler_desc.AddressV = util::to_native(config.address_v);
    sampler_desc.AddressW = util::to_native(config.address_w);
    sampler_desc.MipLODBias = config.lod_bias;
    sampler_desc.MaxAnisotropy = config.max_anisotropy;
    sampler_desc.ComparisonFunc = util::to_native(config.compare_func);

    sampler_desc.MinLOD = config.min_lod;
    sampler_desc.MaxLOD = config.max_lod;

    auto const border_opaque = util::to_opaque_border_color(config.border_color);
    auto const border_alpha = util::to_border_color_alpha(config.border_color);
    sampler_desc.BorderColor[0] = border_opaque;
    sampler_desc.BorderColor[1] = border_opaque;
    sampler_desc.BorderColor[2] = border_opaque;
    sampler_desc.BorderColor[3] = border_alpha;

    return sampler_desc;
}

D3D12_RESOURCE_BARRIER pr::backend::d3d12::util::get_barrier_desc(
    ID3D12Resource* res, pr::backend::resource_state before, pr::backend::resource_state after, int mip_level, int array_slice, unsigned mip_size)
{
    D3D12_RESOURCE_BARRIER out_barrier;
    out_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    out_barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    out_barrier.Transition.pResource = res;
    out_barrier.Transition.StateBefore = util::to_native(before);
    out_barrier.Transition.StateAfter = util::to_native(after);

    if (mip_level == -1)
    {
        CC_ASSERT(array_slice == -1 && "When targetting all MIP levels, all array slices must be targetted");
        out_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    }
    else
    {
        CC_ASSERT(array_slice != -1 && "When specifying target MIP level, array slice must also be specified");
        out_barrier.Transition.Subresource = static_cast<unsigned>(mip_level) + static_cast<unsigned>(array_slice) * mip_size;
    }

    return out_barrier;
}
