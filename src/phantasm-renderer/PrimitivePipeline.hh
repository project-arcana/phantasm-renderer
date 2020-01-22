#pragma once

#include <phantasm-renderer/format.hh>
#include <phantasm-renderer/fwd.hh>
#include <phantasm-renderer/resource_list.hh>

namespace pr
{
// see https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkGraphicsPipelineCreateInfo.html
template <class VertexT, format FragmentF, class BoundResourceList, class... UnboundResources>
class PrimitivePipeline
{
public:
    // TODO: multiple buffers
    template <class BufferT>
    void draw(UnboundResources const&... resources, Buffer<BufferT> const& vertexBuffer)
    {
        static_assert(std::is_same_v<BufferT, VertexT[]>, "incompatible vertex types");
    }

    // TODO: more index types
    template <class BufferT>
    void drawIndexed(UnboundResources const&... resources, Buffer<BufferT> const& vertexBuffer, Buffer<int[]> const& indexBuffer)
    {
        static_assert(std::is_same_v<BufferT, VertexT[]>, "incompatible vertex types");
    }

    template <class... NewResources>
    bound_pipeline_type<VertexT, FragmentF, append_resource_list<BoundResourceList, NewResources...>, remove_resource_prefix<resource_list<UnboundResources...>, NewResources...>>
    bind(NewResources const&... resources)
    {
        static_assert(sizeof...(NewResources) <= sizeof...(UnboundResources), "trying to bind too many resources");
        return {}; // TODO
    }
};
}
