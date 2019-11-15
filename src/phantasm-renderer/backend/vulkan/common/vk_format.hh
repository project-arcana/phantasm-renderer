#pragma once

#include <typed-geometry/detail/comp_traits.hh>

#include <phantasm-renderer/backend/vulkan/loader/volk.hh>

namespace pr::backend::vk::util {

template <class T>
[[nodiscard]] constexpr VkFormat get_vk_format()
{
    if constexpr (tg::is_comp_like<T, 4, float>)
        return VK_FORMAT_R32G32B32A32_SFLOAT;
    else if constexpr (tg::is_comp_like<T, 3, float>)
        return VK_FORMAT_R32G32B32_SFLOAT;
    else if constexpr (tg::is_comp_like<T, 2, float>)
        return VK_FORMAT_R32G32_SFLOAT;
    else if constexpr (tg::is_comp_like<T, 1, float>)
        return VK_FORMAT_R32_SFLOAT;

    else if constexpr (tg::is_comp_like<T, 4, int>)
        return VK_FORMAT_R32G32B32A32_SINT;
    else if constexpr (tg::is_comp_like<T, 3, int>)
        return VK_FORMAT_R32G32B32_SINT;
    else if constexpr (tg::is_comp_like<T, 2, int>)
        return VK_FORMAT_R32G32_SINT;
    else if constexpr (tg::is_comp_like<T, 1, int>)
        return VK_FORMAT_R32_SINT;

    else if constexpr (tg::is_comp_like<T, 4, unsigned>)
        return VK_FORMAT_R32G32B32A32_UINT;
    else if constexpr (tg::is_comp_like<T, 3, unsigned>)
        return VK_FORMAT_R32G32B32_UINT;
    else if constexpr (tg::is_comp_like<T, 2, unsigned>)
        return VK_FORMAT_R32G32_UINT;
    else if constexpr (tg::is_comp_like<T, 1, unsigned>)
        return VK_FORMAT_R32_UINT;

    else
    {
        static_assert(sizeof(T) == 0, "Unknown type");
        return VK_FORMAT_UNDEFINED;
    }
}

template <class T>
static constexpr VkFormat vk_format = get_vk_format<T>();

}
