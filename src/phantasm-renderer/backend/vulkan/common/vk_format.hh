#pragma once

#include <phantasm-renderer/backend/types.hh>

#include <phantasm-renderer/backend/vulkan/loader/volk.hh>

namespace pr::backend::vk::util
{
[[nodiscard]] inline constexpr VkFormat to_vk_format(backend::format format)
{
    using bf = backend::format;
    switch (format)
    {
    case bf::rgba32f:
        return VK_FORMAT_R32G32B32A32_SFLOAT;
    case bf::rgb32f:
        return VK_FORMAT_R32G32B32_SFLOAT;
    case bf::rg32f:
        return VK_FORMAT_R32G32_SFLOAT;
    case bf::r32f:
        return VK_FORMAT_R32_SFLOAT;

    case bf::rgba32i:
        return VK_FORMAT_R32G32B32A32_SINT;
    case bf::rgb32i:
        return VK_FORMAT_R32G32B32_SINT;
    case bf::rg32i:
        return VK_FORMAT_R32G32_SINT;
    case bf::r32i:
        return VK_FORMAT_R32_SINT;

    case bf::rgba32u:
        return VK_FORMAT_R32G32B32A32_UINT;
    case bf::rgb32u:
        return VK_FORMAT_R32G32B32_UINT;
    case bf::rg32u:
        return VK_FORMAT_R32G32_UINT;
    case bf::r32u:
        return VK_FORMAT_R32_UINT;

    case bf::rgba16i:
        return VK_FORMAT_R16G16B16A16_SINT;
    case bf::rgb16i:
        return VK_FORMAT_R16G16B16_SINT;
    case bf::rg16i:
        return VK_FORMAT_R16G16_SINT;
    case bf::r16i:
        return VK_FORMAT_R16_SINT;

    case bf::rgba16u:
        return VK_FORMAT_R16G16B16A16_UINT;
    case bf::rgb16u:
        return VK_FORMAT_R16G16B16_UINT;
    case bf::rg16u:
        return VK_FORMAT_R16G16_UINT;
    case bf::r16u:
        return VK_FORMAT_R16_UINT;

    case bf::rgba16f:
        return VK_FORMAT_R16G16B16A16_SFLOAT;
    case bf::rgb16f:
        return VK_FORMAT_R16G16B16_SFLOAT;
    case bf::rg16f:
        return VK_FORMAT_R16G16_SFLOAT;
    case bf::r16f:
        return VK_FORMAT_R16_SFLOAT;

    case bf::rgba8i:
        return VK_FORMAT_R8G8B8A8_SINT;
    case bf::rgb8i:
        return VK_FORMAT_R8G8B8_SINT;
    case bf::rg8i:
        return VK_FORMAT_R8G8_SINT;
    case bf::r8i:
        return VK_FORMAT_R8_SINT;

    case bf::rgba8u:
        return VK_FORMAT_R8G8B8A8_UINT;
    case bf::rgb8u:
        return VK_FORMAT_R8G8B8_UINT;
    case bf::rg8u:
        return VK_FORMAT_R8G8_UINT;
    case bf::r8u:
        return VK_FORMAT_R8_UINT;

    case bf::rgba8un:
        return VK_FORMAT_R8G8B8A8_UNORM;

    case bf::depth32f:
        return VK_FORMAT_D32_SFLOAT;
    case bf::depth16un:
        return VK_FORMAT_D16_UNORM;
    case bf::depth32f_stencil8u:
        return VK_FORMAT_D32_SFLOAT_S8_UINT;
    case bf::depth24un_stencil8u:
        return VK_FORMAT_D24_UNORM_S8_UINT;
    }
    return VK_FORMAT_UNDEFINED;
}
}
