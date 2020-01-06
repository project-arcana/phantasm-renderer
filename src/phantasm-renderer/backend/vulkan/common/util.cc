#include "util.hh"

#include "vk_format.hh"

cc::capped_vector<VkVertexInputAttributeDescription, 16> pr::backend::vk::util::get_native_vertex_format(cc::span<const pr::backend::vertex_attribute_info> attrib_info)
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

VkVertexInputBindingDescription pr::backend::vk::util::get_vertex_binding(uint32_t vertex_size)
{
    VkVertexInputBindingDescription res = {};
    res.binding = 0;
    res.stride = vertex_size;
    res.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return res;
}
