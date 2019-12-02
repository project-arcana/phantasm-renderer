#pragma once

#include <dxgiformat.h> // This header only contains a single enum, no includes believe it or not

#include <clean-core/assert.hh>

#include <phantasm-renderer/backend/types.hh>

namespace pr::backend::d3d12::util
{
[[nodiscard]] inline constexpr DXGI_FORMAT to_dxgi_format(backend::format format)
{
    using af = backend::format;
    switch (format)
    {
    case af::rgba32f:
        return DXGI_FORMAT_R32G32B32A32_FLOAT;
    case af::rgb32f:
        return DXGI_FORMAT_R32G32B32_FLOAT;
    case af::rg32f:
        return DXGI_FORMAT_R32G32_FLOAT;
    case af::r32f:
        return DXGI_FORMAT_R32_FLOAT;

    case af::rgba32i:
        return DXGI_FORMAT_R32G32B32A32_SINT;
    case af::rgb32i:
        return DXGI_FORMAT_R32G32B32_SINT;
    case af::rg32i:
        return DXGI_FORMAT_R32G32_SINT;
    case af::r32i:
        return DXGI_FORMAT_R32_SINT;

    case af::rgba32u:
        return DXGI_FORMAT_R32G32B32A32_UINT;
    case af::rgb32u:
        return DXGI_FORMAT_R32G32B32_UINT;
    case af::rg32u:
        return DXGI_FORMAT_R32G32_UINT;
    case af::r32u:
        return DXGI_FORMAT_R32_UINT;

    case af::rgba16i:
        return DXGI_FORMAT_R16G16B16A16_SINT;
    case af::rgb16i:
        return DXGI_FORMAT_UNKNOWN;
    case af::rg16i:
        return DXGI_FORMAT_R16G16_SINT;
    case af::r16i:
        return DXGI_FORMAT_R16_SINT;

    case af::rgba16u:
        return DXGI_FORMAT_R16G16B16A16_UINT;
    case af::rgb16u:
        return DXGI_FORMAT_UNKNOWN;
    case af::rg16u:
        return DXGI_FORMAT_R16G16_UINT;
    case af::r16u:
        return DXGI_FORMAT_R16_UINT;

    case af::rgba16f:
        return DXGI_FORMAT_R16G16B16A16_FLOAT;
    case af::rgb16f:
        return DXGI_FORMAT_UNKNOWN;
    case af::rg16f:
        return DXGI_FORMAT_R16G16_FLOAT;
    case af::r16f:
        return DXGI_FORMAT_R16_FLOAT;

    case af::rgba8i:
        return DXGI_FORMAT_R8G8B8A8_SINT;
    case af::rgb8i:
        return DXGI_FORMAT_UNKNOWN;
    case af::rg8i:
        return DXGI_FORMAT_R8G8_SINT;
    case af::r8i:
        return DXGI_FORMAT_R8_SINT;

    case af::rgba8u:
        return DXGI_FORMAT_R8G8B8A8_UINT;
    case af::rgb8u:
        return DXGI_FORMAT_UNKNOWN;
    case af::rg8u:
        return DXGI_FORMAT_R8G8_UINT;
    case af::r8u:
        return DXGI_FORMAT_R8_UINT;

    case af::rgba8un:
        return DXGI_FORMAT_R8G8B8A8_UNORM;
    case af::bgra8un:
        return DXGI_FORMAT_B8G8R8A8_UNORM;

    case af::depth32f:
        return DXGI_FORMAT_D32_FLOAT;
    case af::depth16un:
        return DXGI_FORMAT_D16_UNORM;
    case af::depth32f_stencil8u:
        return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
    case af::depth24un_stencil8u:
        return DXGI_FORMAT_D24_UNORM_S8_UINT;
    }
    return DXGI_FORMAT_UNKNOWN;
}

[[nodiscard]] inline constexpr backend::format to_pr_format(DXGI_FORMAT format)
{
    using af = backend::format;
    switch (format)
    {
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
        return af::rgba32f;
    case DXGI_FORMAT_R32G32B32_FLOAT:
        return af::rgb32f;
    case DXGI_FORMAT_R32G32_FLOAT:
        return af::rg32f;
    case DXGI_FORMAT_R32_FLOAT:
        return af::r32f;

    case DXGI_FORMAT_R32G32B32A32_SINT:
        return af::rgba32i;
    case DXGI_FORMAT_R32G32B32_SINT:
        return af::rgb32i;
    case DXGI_FORMAT_R32G32_SINT:
        return af::rg32i;
    case DXGI_FORMAT_R32_SINT:
        return af::r32i;

    case DXGI_FORMAT_R32G32B32A32_UINT:
        return af::rgba32u;
    case DXGI_FORMAT_R32G32B32_UINT:
        return af::rgb32u;
    case DXGI_FORMAT_R32G32_UINT:
        return af::rg32u;
    case DXGI_FORMAT_R32_UINT:
        return af::r32u;

    case DXGI_FORMAT_R16G16B16A16_SINT:
        return af::rgba16i;
    case DXGI_FORMAT_R16G16_SINT:
        return af::rg16i;
    case DXGI_FORMAT_R16_SINT:
        return af::r16i;

    case DXGI_FORMAT_R16G16B16A16_UINT:
        return af::rgba16u;
    case DXGI_FORMAT_UNKNOWN:
        return af::rgb16u;
    case DXGI_FORMAT_R16G16_UINT:
        return af::rg16u;
    case DXGI_FORMAT_R16_UINT:
        return af::r16u;

    case DXGI_FORMAT_R16G16B16A16_FLOAT:
        return af::rgba16f;
    case DXGI_FORMAT_R16G16_FLOAT:
        return af::rg16f;
    case DXGI_FORMAT_R16_FLOAT:
        return af::r16f;

    case DXGI_FORMAT_R8G8B8A8_SINT:
        return af::rgba8i;
    case DXGI_FORMAT_R8G8_SINT:
        return af::rg8i;
    case DXGI_FORMAT_R8_SINT:
        return af::r8i;

    case DXGI_FORMAT_R8G8B8A8_UINT:
        return af::rgba8u;
    case DXGI_FORMAT_R8G8_UINT:
        return af::rg8u;
    case DXGI_FORMAT_R8_UINT:
        return af::r8u;

    case DXGI_FORMAT_R8G8B8A8_UNORM:
        return af::rgba8un;
    case DXGI_FORMAT_B8G8R8A8_UNORM:
        return af::bgra8un;

    case DXGI_FORMAT_D32_FLOAT:
        return af::depth32f;
    case DXGI_FORMAT_D16_UNORM:
        return af::depth16un;
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        return af::depth32f_stencil8u;
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
        return af::depth24un_stencil8u;

    default:
        CC_ASSERT(false && "untranslatable DXGI_FORMAT");
        return af::rgba8u;
    }
}

[[nodiscard]] inline constexpr unsigned get_dxgi_bytes_per_pixel(DXGI_FORMAT fmt)
{
    switch (fmt)
    {
    case DXGI_FORMAT_BC1_TYPELESS:
    case DXGI_FORMAT_BC1_UNORM:
    case DXGI_FORMAT_BC1_UNORM_SRGB:
    case DXGI_FORMAT_BC4_TYPELESS:
    case DXGI_FORMAT_BC4_UNORM:
    case DXGI_FORMAT_BC4_SNORM:
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
    case DXGI_FORMAT_R16G16B16A16_TYPELESS:
        return 8;

    case DXGI_FORMAT_BC2_TYPELESS:
    case DXGI_FORMAT_BC2_UNORM:
    case DXGI_FORMAT_BC2_UNORM_SRGB:
    case DXGI_FORMAT_BC3_TYPELESS:
    case DXGI_FORMAT_BC3_UNORM:
    case DXGI_FORMAT_BC3_UNORM_SRGB:
    case DXGI_FORMAT_BC5_TYPELESS:
    case DXGI_FORMAT_BC5_UNORM:
    case DXGI_FORMAT_BC5_SNORM:
    case DXGI_FORMAT_BC6H_TYPELESS:
    case DXGI_FORMAT_BC6H_UF16:
    case DXGI_FORMAT_BC6H_SF16:
    case DXGI_FORMAT_BC7_TYPELESS:
    case DXGI_FORMAT_BC7_UNORM:
    case DXGI_FORMAT_BC7_UNORM_SRGB:
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
    case DXGI_FORMAT_R32G32B32A32_TYPELESS:
        return 16;

    case DXGI_FORMAT_R10G10B10A2_TYPELESS:
    case DXGI_FORMAT_R10G10B10A2_UNORM:
    case DXGI_FORMAT_R10G10B10A2_UINT:
    case DXGI_FORMAT_R11G11B10_FLOAT:
    case DXGI_FORMAT_R8G8B8A8_TYPELESS:
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
    case DXGI_FORMAT_R8G8B8A8_UINT:
    case DXGI_FORMAT_R8G8B8A8_SNORM:
    case DXGI_FORMAT_R8G8B8A8_SINT:
    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8X8_UNORM:
    case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
    case DXGI_FORMAT_B8G8R8A8_TYPELESS:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
    case DXGI_FORMAT_B8G8R8X8_TYPELESS:
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
    case DXGI_FORMAT_R16G16_TYPELESS:
    case DXGI_FORMAT_R16G16_FLOAT:
    case DXGI_FORMAT_R16G16_UNORM:
    case DXGI_FORMAT_R16G16_UINT:
    case DXGI_FORMAT_R16G16_SNORM:
    case DXGI_FORMAT_R16G16_SINT:
    case DXGI_FORMAT_R32_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT:
    case DXGI_FORMAT_R32_FLOAT:
    case DXGI_FORMAT_R32_UINT:
    case DXGI_FORMAT_R32_SINT:
        return 4;

    default:
        return 0;
    }
}

[[nodiscard]] inline constexpr unsigned get_dxgi_bits_per_pixel(DXGI_FORMAT fmt)
{
    switch (fmt)
    {
    case DXGI_FORMAT_R32G32B32A32_TYPELESS:
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
    case DXGI_FORMAT_R32G32B32A32_UINT:
    case DXGI_FORMAT_R32G32B32A32_SINT:
        return 128;

    case DXGI_FORMAT_R32G32B32_TYPELESS:
    case DXGI_FORMAT_R32G32B32_FLOAT:
    case DXGI_FORMAT_R32G32B32_UINT:
    case DXGI_FORMAT_R32G32B32_SINT:
        return 96;

    case DXGI_FORMAT_R16G16B16A16_TYPELESS:
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
    case DXGI_FORMAT_R16G16B16A16_UNORM:
    case DXGI_FORMAT_R16G16B16A16_UINT:
    case DXGI_FORMAT_R16G16B16A16_SNORM:
    case DXGI_FORMAT_R16G16B16A16_SINT:
    case DXGI_FORMAT_R32G32_TYPELESS:
    case DXGI_FORMAT_R32G32_FLOAT:
    case DXGI_FORMAT_R32G32_UINT:
    case DXGI_FORMAT_R32G32_SINT:
    case DXGI_FORMAT_R32G8X24_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
    case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
    case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
    case DXGI_FORMAT_Y416:
    case DXGI_FORMAT_Y210:
    case DXGI_FORMAT_Y216:
        return 64;

    case DXGI_FORMAT_R10G10B10A2_TYPELESS:
    case DXGI_FORMAT_R10G10B10A2_UNORM:
    case DXGI_FORMAT_R10G10B10A2_UINT:
    case DXGI_FORMAT_R11G11B10_FLOAT:
    case DXGI_FORMAT_R8G8B8A8_TYPELESS:
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
    case DXGI_FORMAT_R8G8B8A8_UINT:
    case DXGI_FORMAT_R8G8B8A8_SNORM:
    case DXGI_FORMAT_R8G8B8A8_SINT:
    case DXGI_FORMAT_R16G16_TYPELESS:
    case DXGI_FORMAT_R16G16_FLOAT:
    case DXGI_FORMAT_R16G16_UNORM:
    case DXGI_FORMAT_R16G16_UINT:
    case DXGI_FORMAT_R16G16_SNORM:
    case DXGI_FORMAT_R16G16_SINT:
    case DXGI_FORMAT_R32_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT:
    case DXGI_FORMAT_R32_FLOAT:
    case DXGI_FORMAT_R32_UINT:
    case DXGI_FORMAT_R32_SINT:
    case DXGI_FORMAT_R24G8_TYPELESS:
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
    case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
    case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
    case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
    case DXGI_FORMAT_R8G8_B8G8_UNORM:
    case DXGI_FORMAT_G8R8_G8B8_UNORM:
    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8X8_UNORM:
    case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
    case DXGI_FORMAT_B8G8R8A8_TYPELESS:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
    case DXGI_FORMAT_B8G8R8X8_TYPELESS:
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
    case DXGI_FORMAT_AYUV:
    case DXGI_FORMAT_Y410:
    case DXGI_FORMAT_YUY2:
        return 32;

    case DXGI_FORMAT_P010:
    case DXGI_FORMAT_P016:
        return 24;

    case DXGI_FORMAT_R8G8_TYPELESS:
    case DXGI_FORMAT_R8G8_UNORM:
    case DXGI_FORMAT_R8G8_UINT:
    case DXGI_FORMAT_R8G8_SNORM:
    case DXGI_FORMAT_R8G8_SINT:
    case DXGI_FORMAT_R16_TYPELESS:
    case DXGI_FORMAT_R16_FLOAT:
    case DXGI_FORMAT_D16_UNORM:
    case DXGI_FORMAT_R16_UNORM:
    case DXGI_FORMAT_R16_UINT:
    case DXGI_FORMAT_R16_SNORM:
    case DXGI_FORMAT_R16_SINT:
    case DXGI_FORMAT_B5G6R5_UNORM:
    case DXGI_FORMAT_B5G5R5A1_UNORM:
    case DXGI_FORMAT_A8P8:
    case DXGI_FORMAT_B4G4R4A4_UNORM:
        return 16;

    case DXGI_FORMAT_NV12:
    case DXGI_FORMAT_420_OPAQUE:
    case DXGI_FORMAT_NV11:
        return 12;

    case DXGI_FORMAT_R8_TYPELESS:
    case DXGI_FORMAT_R8_UNORM:
    case DXGI_FORMAT_R8_UINT:
    case DXGI_FORMAT_R8_SNORM:
    case DXGI_FORMAT_R8_SINT:
    case DXGI_FORMAT_A8_UNORM:
    case DXGI_FORMAT_AI44:
    case DXGI_FORMAT_IA44:
    case DXGI_FORMAT_P8:
        return 8;

    case DXGI_FORMAT_R1_UNORM:
        return 1;

    case DXGI_FORMAT_BC1_TYPELESS:
    case DXGI_FORMAT_BC1_UNORM:
    case DXGI_FORMAT_BC1_UNORM_SRGB:
    case DXGI_FORMAT_BC4_TYPELESS:
    case DXGI_FORMAT_BC4_UNORM:
    case DXGI_FORMAT_BC4_SNORM:
        return 4;

    case DXGI_FORMAT_BC2_TYPELESS:
    case DXGI_FORMAT_BC2_UNORM:
    case DXGI_FORMAT_BC2_UNORM_SRGB:
    case DXGI_FORMAT_BC3_TYPELESS:
    case DXGI_FORMAT_BC3_UNORM:
    case DXGI_FORMAT_BC3_UNORM_SRGB:
    case DXGI_FORMAT_BC5_TYPELESS:
    case DXGI_FORMAT_BC5_UNORM:
    case DXGI_FORMAT_BC5_SNORM:
    case DXGI_FORMAT_BC6H_TYPELESS:
    case DXGI_FORMAT_BC6H_UF16:
    case DXGI_FORMAT_BC6H_SF16:
    case DXGI_FORMAT_BC7_TYPELESS:
    case DXGI_FORMAT_BC7_UNORM:
    case DXGI_FORMAT_BC7_UNORM_SRGB:
        return 8;

    default:
        return 0;
    }
}

[[nodiscard]] inline constexpr bool is_dxt_format(DXGI_FORMAT fmt) { return (fmt >= DXGI_FORMAT_BC1_TYPELESS) && (fmt <= DXGI_FORMAT_BC5_SNORM); }

[[nodiscard]] inline constexpr DXGI_FORMAT to_srgb_format(DXGI_FORMAT fmt)
{
    switch (fmt)
    {
    case DXGI_FORMAT_R8G8B8A8_UNORM:
        return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    case DXGI_FORMAT_BC1_UNORM:
        return DXGI_FORMAT_BC1_UNORM_SRGB;
    case DXGI_FORMAT_BC2_UNORM:
        return DXGI_FORMAT_BC2_UNORM_SRGB;
    case DXGI_FORMAT_BC3_UNORM:
        return DXGI_FORMAT_BC3_UNORM_SRGB;
    case DXGI_FORMAT_B8G8R8A8_UNORM:
        return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
    case DXGI_FORMAT_B8G8R8X8_UNORM:
        return DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;
    case DXGI_FORMAT_BC7_UNORM:
        return DXGI_FORMAT_BC7_UNORM_SRGB;
    default:
        return fmt;
    }
}
}
