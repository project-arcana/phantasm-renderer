#pragma once

#include <clean-core/capped_vector.hh>

#include <reflector/members.hh>

#include <phantasm-renderer/backend/vulkan/common/vk_format.hh>
#include <phantasm-renderer/backend/vulkan/loader/volk.hh>

namespace pr::backend::vk
{
namespace detail
{
template <class VertT>
struct vertex_visitor
{
    // TODO: Eventually, this will work once the 19.23 bug is fixed
    static constexpr auto num_attributes = rf::member_count<VertT>;
    cc::capped_vector<VkVertexInputAttributeDescription, num_attributes> attributes;

    template <class T>
    void operator()(T const& ref, char const*)
    {
        VkVertexInputAttributeDescription& attr = attributes.emplace_back();
        attr.binding = 0;
        attr.location = attributes.size() - 1;
        attr.format = util::vk_format<T>;

        // This call assumes that the original VertT& in get_vertex_attributes is a dereferenced nullptr
        attr.offset = uint32_t(reinterpret_cast<size_t>(&ref));
    }
};
}

template <class VertT>
[[nodiscard]] auto get_vertex_attributes()
{
    detail::vertex_visitor<VertT> visitor;
    VertT* volatile dummyPointer = nullptr; // Volatile to avoid UB-based optimization
    introspect(visitor, *dummyPointer);
    return visitor.attributes;
}

template <class VertT>
[[nodiscard]] VkVertexInputBindingDescription get_vertex_binding()
{
    VkVertexInputBindingDescription res = {};
    res.binding = 0;
    res.stride = sizeof(VertT);
    res.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return res;
}
}
