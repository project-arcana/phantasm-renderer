#pragma once

#include <phantasm-renderer/backend/types.hh>

#include <phantasm-renderer/backend/vulkan/loader/volk.hh>

namespace pr::backend::vk::util
{
[[nodiscard]] inline constexpr VkFormat to_vk_format(assets::attribute_format format)
{
    using af = assets::attribute_format;
    switch (format)
    {
    case af::rgba32f:
        return VK_FORMAT_R32G32B32A32_SFLOAT;
    case af::rgb32f:
        return VK_FORMAT_R32G32B32_SFLOAT;
    case af::rg32f:
        return VK_FORMAT_R32G32_SFLOAT;
    case af::r32f:
        return VK_FORMAT_R32_SFLOAT;

    case af::rgba32i:
        return VK_FORMAT_R32G32B32A32_SINT;
    case af::rgb32i:
        return VK_FORMAT_R32G32B32_SINT;
    case af::rg32i:
        return VK_FORMAT_R32G32_SINT;
    case af::r32i:
        return VK_FORMAT_R32_SINT;

    case af::rgba32u:
        return VK_FORMAT_R32G32B32A32_UINT;
    case af::rgb32u:
        return VK_FORMAT_R32G32B32_UINT;
    case af::rg32u:
        return VK_FORMAT_R32G32_UINT;
    case af::r32u:
        return VK_FORMAT_R32_UINT;

    case af::rgba16i:
        return VK_FORMAT_R16G16B16A16_SINT;
    case af::rgb16i:
        return VK_FORMAT_R16G16B16_SINT;
    case af::rg16i:
        return VK_FORMAT_R16G16_SINT;
    case af::r16i:
        return VK_FORMAT_R16_SINT;

    case af::rgba16u:
        return VK_FORMAT_R16G16B16A16_UINT;
    case af::rgb16u:
        return VK_FORMAT_R16G16B16_UINT;
    case af::rg16u:
        return VK_FORMAT_R16G16_UINT;
    case af::r16u:
        return VK_FORMAT_R16_UINT;

    case af::rgba8i:
        return VK_FORMAT_R8G8B8A8_SINT;
    case af::rgb8i:
        return VK_FORMAT_R8G8B8_SINT;
    case af::rg8i:
        return VK_FORMAT_R8G8_SINT;
    case af::r8i:
        return VK_FORMAT_R8_SINT;

    case af::rgba8u:
        return VK_FORMAT_R8G8B8A8_UINT;
    case af::rgb8u:
        return VK_FORMAT_R8G8B8_UINT;
    case af::rg8u:
        return VK_FORMAT_R8G8_UINT;
    case af::r8u:
        return VK_FORMAT_R8_UINT;
    }
    return VK_FORMAT_UNDEFINED;
}
}
