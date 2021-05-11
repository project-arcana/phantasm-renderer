#pragma once

#include <phantasm-hardware-interface/commands.hh>

#include <phantasm-renderer/argument.hh>
#include <phantasm-renderer/common/api.hh>
#include <phantasm-renderer/fwd.hh>
#include <phantasm-renderer/resource_types.hh>

namespace pr::raii
{
class Frame;

class PR_API GraphicsPass
{
public:
    [[nodiscard]] GraphicsPass bind(prebuilt_argument const& sv)
    {
        GraphicsPass p = {mParent, mCmd, mArgNum};
        p.add_argument(sv._sv, phi::handle::null_resource, 0);
        return p;
    }

    [[nodiscard]] GraphicsPass bind(prebuilt_argument const& sv, buffer const& constant_buffer, uint32_t constant_buffer_offset = 0)
    {
        GraphicsPass p = {mParent, mCmd, mArgNum};
        p.add_argument(sv._sv, constant_buffer.handle, constant_buffer_offset);
        return p;
    }

    // CBV only
    [[nodiscard]] GraphicsPass bind(buffer const& constant_buffer, uint32_t constant_buffer_offset = 0)
    {
        GraphicsPass p = {mParent, mCmd, mArgNum};
        p.add_argument(phi::handle::null_shader_view, constant_buffer.handle, constant_buffer_offset);
        return p;
    }

    // raw phi
    [[nodiscard]] GraphicsPass bind(phi::handle::shader_view sv, phi::handle::resource cbv = phi::handle::null_resource, uint32_t cbv_offset = 0)
    {
        GraphicsPass p = {mParent, mCmd, mArgNum};
        p.add_argument(sv, cbv, cbv_offset);
        return p;
    }

    // cache-access variants
    // hits a OS mutex
    [[nodiscard]] GraphicsPass bind(argument& arg, phi::handle::resource constant_buffer = phi::handle::null_resource, uint32_t constant_buffer_offset = 0)
    {
        GraphicsPass p = {mParent, mCmd, mArgNum};
        p.add_cached_argument(arg, constant_buffer, constant_buffer_offset);
        return p;
    }

    [[nodiscard]] GraphicsPass bind(argument& arg, buffer const& constant_buffer, uint32_t constant_buffer_offset = 0)
    {
        GraphicsPass p = {mParent, mCmd, mArgNum};
        p.add_cached_argument(arg, constant_buffer.handle, constant_buffer_offset);
        return p;
    }

    // draw without vertices
    void draw(uint32_t num_vertices, uint32_t num_instances = 1);

    // draw vertices
    void draw(buffer const& vertex_buffer, uint32_t num_instances = 1, int vertex_offset = 0);

    // indexed draw
    void draw(buffer const& vertex_buffer, buffer const& index_buffer, uint32_t num_instances = 1, int vertex_offset = 0, uint32_t index_offset = 0);

    // indexed draw with raw handles
    void draw(phi::handle::resource vertex_buffer, phi::handle::resource index_buffer, uint32_t num_indices, uint32_t num_instances = 1, int vertex_offset = 0, uint32_t index_offset = 0);

    // indexed draw with raw handles and up to 4 vertex buffers
    void draw(cc::span<phi::handle::resource const> vertex_buffers,
              phi::handle::resource index_buffer,
              uint32_t num_indices,
              uint32_t num_instances = 1,
              int vertex_offset = 0,
              uint32_t index_offset = 0);

    // indirect draw
    void draw_indirect(buffer const& argument_buffer, buffer const& vertex_buffer, uint32_t num_args, uint32_t arg_buffer_offset_bytes = 0);

    // indirect indexed draw
    void draw_indirect(buffer const& argument_buffer, buffer const& vertex_buffer, buffer const& index_buffer, uint32_t num_args, uint32_t arg_buffer_offset_bytes = 0);

    // indirect draw with raw handles
    void draw_indirect(phi::handle::resource argument_buffer,
                       phi::handle::resource vertex_buffer,
                       phi::handle::resource index_buffer,
                       uint32_t num_args,
                       uint32_t arg_buffer_offset_bytes = 0);

    void set_scissor(tg::iaabb2 scissor) { mCmd.scissor = scissor; }
    void set_scissor(int left, int top, int right, int bot) { mCmd.scissor = tg::iaabb2({left, top}, {right, bot}); }
    void unset_scissor() { mCmd.scissor = tg::iaabb2(-1, -1); }

    void set_constant_buffer(buffer const& constant_buffer, uint32_t offset = 0);
    void set_constant_buffer(phi::handle::resource raw_cbv, uint32_t offset = 0);

    void set_constant_buffer_offset(uint32_t offset);


    template <class T>
    void write_constants(T const& val)
    {
        mCmd.write_root_constants<T>(val);
    }

    /// NOTE: advanced usage
    phi::cmd::draw& raw_command() { return mCmd; }

    /// NOTE: advanced usage
    void reset_pipeline_state(phi::handle::pipeline_state pso) { mCmd.pipeline_state = pso; }

    GraphicsPass(GraphicsPass const&) = delete;
    GraphicsPass(GraphicsPass&& rhs) noexcept = default;

private:
    friend class Framebuffer;
    // Framebuffer-side ctor
    GraphicsPass(Frame* parent, phi::handle::pipeline_state pso) : mParent(parent) { mCmd.pipeline_state = pso; }

private:
    // internal re-bind ctor
    GraphicsPass(Frame* parent, phi::cmd::draw const& cmd, uint32_t arg_i) : mParent(parent), mCmd(cmd), mArgNum(arg_i) {}

private:
    // persisted, raw phi
    void add_argument(phi::handle::shader_view sv, phi::handle::resource cbv, uint32_t cbv_offset);

    // cache-access variant
    // hits a OS mutex
    void add_cached_argument(argument& arg, phi::handle::resource cbv, uint32_t cbv_offset);

    Frame* mParent = nullptr;
    phi::cmd::draw mCmd;
    // index of owning argument - 1, 0 means no arguments existing
    uint32_t mArgNum = 0;
};

// inline implementation

inline void GraphicsPass::set_constant_buffer(const buffer& constant_buffer, uint32_t offset)
{
    set_constant_buffer(constant_buffer.handle, offset);
}

inline void GraphicsPass::set_constant_buffer(phi::handle::resource raw_cbv, uint32_t offset)
{
    CC_ASSERT(mArgNum != 0 && "Attempted to set_constant_buffer on a GraphicsPass without prior bind");
    mCmd.shader_arguments[uint8_t(mArgNum - 1)].constant_buffer = raw_cbv;
    mCmd.shader_arguments[uint8_t(mArgNum - 1)].constant_buffer_offset = offset;
}

inline void GraphicsPass::set_constant_buffer_offset(uint32_t offset)
{
    CC_ASSERT(mArgNum != 0 && "Attempted to set_constant_buffer_offset on a GraphicsPass without prior bind");
    mCmd.shader_arguments[uint8_t(mArgNum - 1)].constant_buffer_offset = offset;
}

inline void GraphicsPass::add_argument(phi::handle::shader_view sv, phi::handle::resource cbv, uint32_t cbv_offset)
{
    ++mArgNum;
    mCmd.add_shader_arg(cbv, cbv_offset, sv);
}
}
