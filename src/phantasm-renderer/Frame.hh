#pragma once

#include <phantasm-hardware-interface/commands.hh>

#include <phantasm-renderer/Buffer.hh>
#include <phantasm-renderer/FragmentShader.hh>
#include <phantasm-renderer/Image.hh>
#include <phantasm-renderer/Pass.hh>
#include <phantasm-renderer/VertexShader.hh>
#include <phantasm-renderer/common/growing_writer.hh>
#include <phantasm-renderer/default_config.hh>
#include <phantasm-renderer/format.hh>
#include <phantasm-renderer/fragment_type.hh>
#include <phantasm-renderer/fwd.hh>
#include <phantasm-renderer/immediate.hh>
#include <phantasm-renderer/resources.hh>
#include <phantasm-renderer/vertex_type.hh>

#include <clean-core/span.hh>
#include <clean-core/string_view.hh>

#include <typed-geometry/tg.hh>

namespace pr
{
class Frame
{
    // pass RAII API
public:
    template <format FragmentF, format DepthF>
    Pass<FragmentF, empty_resource_list> render_to(Image<2, FragmentF> const& img, Image<2, DepthF> const& depth_buffer)
    {
        phi::cmd::begin_render_pass brp;
        brp.set_2d_depth_stencil(depth_buffer.getHandle(), DepthF, phi::rt_clear_type::clear, depth_buffer.getRTInfo().num_samples > 1);
        brp.add_2d_rt(img.getHandle(), FragmentF, phi::rt_clear_type::clear, img.getRTInfo().num_samples > 1);
        brp.viewport = tg::min(img.size(), depth_buffer.size());
        mWriter.add_command(brp);

        return {mCtx, &mWriter};
    }
    template <format FragmentF, format DepthF>
    Pass<FragmentF, empty_resource_list> render_to(Image<2, FragmentF> const& img, Image<2, DepthF> const& depth_buffer, tg::isize2 viewport)
    {
        phi::cmd::begin_render_pass brp;
        brp.set_2d_depth_stencil(depth_buffer.getHandle(), DepthF, phi::rt_clear_type::clear, depth_buffer.getRTInfo().num_samples > 1);
        brp.add_2d_rt(img.getHandle(), FragmentF, phi::rt_clear_type::clear, img.getRTInfo().num_samples > 1);
        brp.viewport = viewport;
        mWriter.add_command(brp);

        return {mCtx, &mWriter};
    }

    // move-only type
public:
    Frame(Frame const&) = delete;
    Frame(Frame&&) noexcept = default;
    Frame& operator=(Frame const&) = delete;
    Frame& operator=(Frame&&) noexcept = default;

private:
    Frame(Context* ctx, size_t size) : mCtx(ctx), mWriter(size) {}
    friend class Context;

    std::byte* getMemory() const { return mWriter.buffer(); }
    size_t getSize() const { return mWriter.size(); }
    bool isEmpty() const { return mWriter.is_empty(); }

    // members
private:
    Context* mCtx;
    growing_writer mWriter;
};
}
