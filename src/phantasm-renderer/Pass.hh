#pragma once

#include <type_traits>

#include <functional> // TODO: replace with cc::function

#include <rich-log/log.hh>

#include <phantasm-renderer/common/growing_writer.hh>
#include <phantasm-renderer/fwd.hh>
#include <phantasm-renderer/resource_list.hh>

namespace pr
{
template <format FragmentF, class BoundResourceList>
class Pass
{
public:
    template <class... UnboundResources, class VertexT, format ShaderFragmentF>
    PrimitivePipeline<VertexT, ShaderFragmentF, BoundResourceList, UnboundResources...> pipeline(VertexShader<VertexT> const& vs,
                                                                                                 FragmentShader<ShaderFragmentF> const& fs,
                                                                                                 phi::pipeline_config const& cfg)
    {
        static_assert(FragmentF == ShaderFragmentF, "incompatible fragment types");
        return {mCtx, mParentWriter, vs, fs, cfg};
    }

    template <class... NewResources>
    Pass<FragmentF, append_resource_list<BoundResourceList, NewResources...>> bind(NewResources const&... resources)
    {
        static_assert(sizeof...(NewResources) > 0, "must bind at least one new resource");
        return {mCtx, mParentWriter};
    }

    template <int D, format F, class T>
    void clear(Image<D, F> const& img, T const& value) // TODO: value type should be translated once
    {
        LOG(warning)("Pass::clear unimplemented");
        // TODO: only for attached images or not?
    }

public:
    Pass(Context* ctx, growing_writer* writer) : mCtx(ctx), mParentWriter(writer) {}

    Pass(Pass const&) = delete;
    Pass(Pass&&) noexcept = delete;
    Pass& operator=(Pass const&) = delete;
    Pass& operator=(Pass&&) noexcept = delete; // TODO: allow move

    ~Pass() { mParentWriter->add_command(phi::cmd::end_render_pass{}); }

private:
    Context* mCtx;
    growing_writer* mParentWriter;
};
}
