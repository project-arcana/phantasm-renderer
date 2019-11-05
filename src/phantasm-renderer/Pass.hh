#pragma once

#include <type_traits>

#include <functional> // TODO: replace with cc::function

#include <phantasm-renderer/fwd.hh>
#include <phantasm-renderer/primitive_pipeline_config.hh>
#include <phantasm-renderer/resource_list.hh>

namespace pr
{
template <class FragmentT, class BoundResourceList>
class Pass
{
public:
    template <class... UnboundResources, class VertexT, class ShaderFragmentT>
    PrimitivePipeline<VertexT, ShaderFragmentT, BoundResourceList, UnboundResources...> pipeline(VertexShader<VertexT> const& vs,
                                                                                                 FragmentShader<ShaderFragmentT> const& fs,
                                                                                                 primitive_pipeline_config const& cfg)
    {
        static_assert(std::is_same_v<FragmentT, ShaderFragmentT>, "incompatible fragment types");
        return {}; // TODO
    }

    template <class... NewResources>
    Pass<FragmentT, append_resource_list<BoundResourceList, NewResources...>> bind(NewResources const&... resources)
    {
        static_assert(sizeof...(NewResources) > 0, "must bind at least one new resource");
        return {}; // TODO
    }

    template <int D, class T, bool IsLocal>
    void clear(Image<D, T, IsLocal> const& img, T const& value) // TODO: value type should be translated once
    {
        // TODO: only for attached images or not?
    }

    void defer(std::function<void(Pass&)> f)
    {
        // TODO
    }
};
}
