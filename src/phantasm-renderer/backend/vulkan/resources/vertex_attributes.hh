#pragma once

#include <clean-core/capped_vector.hh>

#include <phantasm-renderer/backend/assets/vertex_attrib_info.hh>

#include <phantasm-renderer/backend/vulkan/common/vk_format.hh>
#include <phantasm-renderer/backend/vulkan/loader/volk.hh>

namespace pr::backend::vk
{
[[nodiscard]] inline cc::capped_vector<VkVertexInputAttributeDescription, 16> get_input_description(cc::span<vertex_attribute_info const> attrib_info)
{
    cc::capped_vector<VkVertexInputAttributeDescription, 16> res;
    for (auto const& ai : attrib_info)
    {
        VkVertexInputAttributeDescription& elem = res.emplace_back();
        elem.binding = 0;
        elem.location = uint32_t(res.size() - 1);
        elem.format = util::to_vk_format(ai.format);
        elem.offset = ai.offset;
    }

    return res;
}

[[nodiscard]] inline VkVertexInputBindingDescription get_vertex_binding(uint32_t vertex_size)
{
    VkVertexInputBindingDescription res = {};
    res.binding = 0;
    res.stride = vertex_size;
    res.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return res;
}
}
