#pragma once
#ifdef PR_BACKEND_D3D12

#include <dxgiformat.h> // This header only contains a single enum, no includes believe it or not

#include <typed-geometry/detail/comp_traits.hh>

namespace pr::backend::d3d12::util
{
template <class T>
[[nodiscard]] constexpr DXGI_FORMAT get_dxgi_format()
{
    if constexpr (tg::is_comp_like<T, 4, float>)
        return DXGI_FORMAT_R32G32B32_FLOAT;
    else if constexpr (tg::is_comp_like<T, 3, float>)
        return DXGI_FORMAT_R32G32B32_FLOAT;
    else if constexpr (tg::is_comp_like<T, 2, float>)
        return DXGI_FORMAT_R32G32_FLOAT;
    else if constexpr (tg::is_comp_like<T, 1, float>)
        return DXGI_FORMAT_R32_FLOAT;

    else if constexpr (tg::is_comp_like<T, 4, int>)
        return DXGI_FORMAT_R32G32B32_SINT;
    else if constexpr (tg::is_comp_like<T, 3, int>)
        return DXGI_FORMAT_R32G32B32_SINT;
    else if constexpr (tg::is_comp_like<T, 2, int>)
        return DXGI_FORMAT_R32G32_SINT;
    else if constexpr (tg::is_comp_like<T, 1, int>)
        return DXGI_FORMAT_R32_SINT;

    else if constexpr (tg::is_comp_like<T, 4, unsigned>)
        return DXGI_FORMAT_R32G32B32_UINT;
    else if constexpr (tg::is_comp_like<T, 3, unsigned>)
        return DXGI_FORMAT_R32G32B32_UINT;
    else if constexpr (tg::is_comp_like<T, 2, unsigned>)
        return DXGI_FORMAT_R32G32_UINT;
    else if constexpr (tg::is_comp_like<T, 1, unsigned>)
        return DXGI_FORMAT_R32_UINT;

    else
    {
        static_assert(sizeof(T) == 0, "Unknown type");
        return DXGI_FORMAT_UNKNOWN;
    }
}

template <class T>
static constexpr DXGI_FORMAT dxgi_format = get_dxgi_format<T>();


[[nodiscard]] inline constexpr unsigned get_dxgi_size(DXGI_FORMAT fmt)
{
    switch (fmt)
    {
    case (DXGI_FORMAT_BC1_TYPELESS):
    case (DXGI_FORMAT_BC1_UNORM):
    case (DXGI_FORMAT_BC1_UNORM_SRGB):
    case (DXGI_FORMAT_BC4_TYPELESS):
    case (DXGI_FORMAT_BC4_UNORM):
    case (DXGI_FORMAT_BC4_SNORM):
    case (DXGI_FORMAT_R16G16B16A16_FLOAT):
    case (DXGI_FORMAT_R16G16B16A16_TYPELESS):
        return 8;

    case (DXGI_FORMAT_BC2_TYPELESS):
    case (DXGI_FORMAT_BC2_UNORM):
    case (DXGI_FORMAT_BC2_UNORM_SRGB):
    case (DXGI_FORMAT_BC3_TYPELESS):
    case (DXGI_FORMAT_BC3_UNORM):
    case (DXGI_FORMAT_BC3_UNORM_SRGB):
    case (DXGI_FORMAT_BC5_TYPELESS):
    case (DXGI_FORMAT_BC5_UNORM):
    case (DXGI_FORMAT_BC5_SNORM):
    case (DXGI_FORMAT_BC6H_TYPELESS):
    case (DXGI_FORMAT_BC6H_UF16):
    case (DXGI_FORMAT_BC6H_SF16):
    case (DXGI_FORMAT_BC7_TYPELESS):
    case (DXGI_FORMAT_BC7_UNORM):
    case (DXGI_FORMAT_BC7_UNORM_SRGB):
    case (DXGI_FORMAT_R32G32B32A32_FLOAT):
    case (DXGI_FORMAT_R32G32B32A32_TYPELESS):
        return 16;

    case (DXGI_FORMAT_R10G10B10A2_TYPELESS):
    case (DXGI_FORMAT_R10G10B10A2_UNORM):
    case (DXGI_FORMAT_R10G10B10A2_UINT):
    case (DXGI_FORMAT_R11G11B10_FLOAT):
    case (DXGI_FORMAT_R8G8B8A8_TYPELESS):
    case (DXGI_FORMAT_R8G8B8A8_UNORM):
    case (DXGI_FORMAT_R8G8B8A8_UNORM_SRGB):
    case (DXGI_FORMAT_R8G8B8A8_UINT):
    case (DXGI_FORMAT_R8G8B8A8_SNORM):
    case (DXGI_FORMAT_R8G8B8A8_SINT):
    case (DXGI_FORMAT_B8G8R8A8_UNORM):
    case (DXGI_FORMAT_B8G8R8X8_UNORM):
    case (DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM):
    case (DXGI_FORMAT_B8G8R8A8_TYPELESS):
    case (DXGI_FORMAT_B8G8R8A8_UNORM_SRGB):
    case (DXGI_FORMAT_B8G8R8X8_TYPELESS):
    case (DXGI_FORMAT_B8G8R8X8_UNORM_SRGB):
    case (DXGI_FORMAT_R16G16_TYPELESS):
    case (DXGI_FORMAT_R16G16_FLOAT):
    case (DXGI_FORMAT_R16G16_UNORM):
    case (DXGI_FORMAT_R16G16_UINT):
    case (DXGI_FORMAT_R16G16_SNORM):
    case (DXGI_FORMAT_R16G16_SINT):
    case (DXGI_FORMAT_R32_TYPELESS):
    case (DXGI_FORMAT_D32_FLOAT):
    case (DXGI_FORMAT_R32_FLOAT):
    case (DXGI_FORMAT_R32_UINT):
    case (DXGI_FORMAT_R32_SINT):
        return 4;

    default:
        return 0;
    }
}

[[nodiscard]] inline constexpr bool is_dxt_format(DXGI_FORMAT fmt) { return (fmt >= DXGI_FORMAT_BC1_TYPELESS) && (fmt <= DXGI_FORMAT_BC5_SNORM); }

[[nodiscard]] inline constexpr DXGI_FORMAT translate_srgb_format(DXGI_FORMAT fmt, bool use_srgb)
{
    if (use_srgb)
    {
        switch (fmt)
        {
        case DXGI_FORMAT_B8G8R8A8_UNORM:
            return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
        case DXGI_FORMAT_R8G8B8A8_UNORM:
            return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        case DXGI_FORMAT_BC1_UNORM:
            return DXGI_FORMAT_BC1_UNORM_SRGB;
        case DXGI_FORMAT_BC2_UNORM:
            return DXGI_FORMAT_BC2_UNORM_SRGB;
        case DXGI_FORMAT_BC3_UNORM:
            return DXGI_FORMAT_BC3_UNORM_SRGB;
        case DXGI_FORMAT_BC7_UNORM:
            return DXGI_FORMAT_BC7_UNORM_SRGB;
        default:
            return DXGI_FORMAT_UNKNOWN;
        }
    }

    return fmt;
}
}

#endif
