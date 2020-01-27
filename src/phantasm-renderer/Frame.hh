#pragma once

#include <phantasm-hardware-interface/commands.hh>

#include <phantasm-renderer/Buffer.hh>
#include <phantasm-renderer/FragmentShader.hh>
#include <phantasm-renderer/Image.hh>
#include <phantasm-renderer/Pass.hh>
#include <phantasm-renderer/VertexShader.hh>
#include <phantasm-renderer/default_config.hh>
#include <phantasm-renderer/format.hh>
#include <phantasm-renderer/fragment_type.hh>
#include <phantasm-renderer/fwd.hh>
#include <phantasm-renderer/immediate.hh>
#include <phantasm-renderer/resources.hh>
#include <phantasm-renderer/vertex_type.hh>

#include <clean-core/span.hh>
#include <clean-core/string_view.hh>

#include <typed-geometry/tg-lean.hh>

namespace pr
{
class Frame
{
    // creation API
public:
    template <format FragmentF>
    FragmentShader<FragmentF> make_fragment_shader(cc::string_view code)
    {
        return {};
    }

    //    template <class... FragmentT>
    //    FragmentShader<fragment_type_of<FragmentT...>> make_fragment_shader(cc::string_view code)
    //    {
    //        return {}; // TODO
    //    }
    //    template <format FragmentF> // must be set to void!
    //    FragmentShader<void> make_fragment_shader()
    //    {
    //        //static_assert(std::is_same_v<FragmentF, void>, "empty fragment shader must be void");
    //        return {}; // TODO
    //    }
    template <class... VertexT>
    VertexShader<vertex_type_of<VertexT...>> make_vertex_shader(cc::string_view code)
    {
        return {}; // TODO
    }

    // pass RAII API
public:
    template <int D, format FragmentF, format DepthF>
    Pass<FragmentF, empty_resource_list> render_to(Image<D, FragmentF> const& img, Image<D, DepthF> const& depth_buffer)
    {
        return {}; // TODO
    }
    template <int D, format FragmentF, format DepthF>
    Pass<FragmentF, empty_resource_list> render_to(Image<D, FragmentF> const& img, Image<D, DepthF> const& depth_buffer, tg::iaabb2 viewport)
    {
        return {}; // TODO
    }

    // move-only type
public:
    Frame(Frame const&) = delete;
    Frame(Frame&&) noexcept = default;
    Frame& operator=(Frame const&) = delete;
    Frame& operator=(Frame&&) noexcept = default;

private:
    Frame(Context* ctx, size_t size)
      : mContext(ctx), mCmdlistMemory(static_cast<std::byte*>(std::malloc(size))), mCmdlistSizeBytes(size), mCmdWriter(mCmdlistMemory, mCmdlistSizeBytes)
    {
    }
    friend class Context;

    std::byte* getMemory() const { return mCmdWriter.buffer(); }
    size_t getSize() const { return mCmdWriter.size(); }
    bool isEmpty() const { return mCmdWriter.empty(); }

    // members
private:
    Context* mContext;
    std::byte* mCmdlistMemory;
    size_t mCmdlistSizeBytes;
    phi::command_stream_writer mCmdWriter;
};
}
