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
    template <class... Args>
    [[nodiscard]] GraphicsPass bind(Args&&... args)
    {
        GraphicsPass p = {mParent, mCmd, mArgNum};
        p.add_argument(cc::forward<Args>(args)...);
        return p;
    }

    void draw(unsigned num_vertices, unsigned num_instances = 1);
    void draw(buffer const& vertex_buffer, unsigned num_instances = 1);
    void draw(buffer const& vertex_buffer, buffer const& index_buffer, unsigned num_instances = 1);
    void draw(phi::handle::resource vertex_buffer, phi::handle::resource index_buffer, unsigned num_indices, unsigned num_instances = 1);

    void draw_indirect(buffer const& argument_buffer, buffer const& vertex_buffer, unsigned num_args, unsigned arg_buffer_offset = 0);

    void set_offset(int vertex_offset, unsigned index_offset = 0);

    void set_scissor(tg::iaabb2 scissor) { mCmd.scissor = scissor; }
    void set_scissor(int left, int top, int right, int bot) { mCmd.scissor = tg::iaabb2({left, top}, {right, bot}); }
    void unset_scissor() { mCmd.scissor = tg::iaabb2(-1, -1); }

    void set_constant_buffer(buffer const& constant_buffer, unsigned offset = 0);
    void set_constant_buffer(phi::handle::resource raw_cbv, unsigned offset = 0);

    void set_constant_buffer_offset(unsigned offset);


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
    GraphicsPass(Frame* parent, phi::cmd::draw const& cmd, unsigned arg_i) : mParent(parent), mCmd(cmd), mArgNum(arg_i) {}

private:
    // cache-access variants
    // hits a OS mutex
    void add_argument(argument const& arg);
    void add_argument(argument const& arg, buffer const& constant_buffer, uint32_t constant_buffer_offset = 0);

    // persisted variants
    void add_argument(prebuilt_argument const& sv);
    void add_argument(prebuilt_argument const& sv, buffer const& constant_buffer, uint32_t constant_buffer_offset = 0);

    // raw phi
    void add_argument(phi::handle::shader_view sv, phi::handle::resource cbv = phi::handle::null_resource, uint32_t cbv_offset = 0);

    void add_argument(buffer const& constant_buffer, uint32_t constant_buffer_offset = 0);

    Frame* mParent = nullptr;
    phi::cmd::draw mCmd;
    // index of owning argument - 1, 0 means no arguments existing
    unsigned mArgNum = 0;
};

// inline implementation

inline void GraphicsPass::set_offset(int vertex_offset, unsigned index_offset)
{
    mCmd.vertex_offset = vertex_offset;
    mCmd.index_offset = index_offset;
}

inline void GraphicsPass::set_constant_buffer(const buffer& constant_buffer, unsigned offset)
{
    set_constant_buffer(constant_buffer.res.handle, offset);
}

inline void GraphicsPass::set_constant_buffer(phi::handle::resource raw_cbv, unsigned offset)
{
    CC_ASSERT(mArgNum != 0 && "Attempted to set_constant_buffer on a GraphicsPass without prior bind");
    mCmd.shader_arguments[uint8_t(mArgNum - 1)].constant_buffer = raw_cbv;
    mCmd.shader_arguments[uint8_t(mArgNum - 1)].constant_buffer_offset = offset;
}

inline void GraphicsPass::set_constant_buffer_offset(unsigned offset)
{
    CC_ASSERT(mArgNum != 0 && "Attempted to set_constant_buffer_offset on a GraphicsPass without prior bind");
    mCmd.shader_arguments[uint8_t(mArgNum - 1)].constant_buffer_offset = offset;
}

inline void GraphicsPass::add_argument(prebuilt_argument const& sv)
{
    ++mArgNum;
    mCmd.add_shader_arg(phi::handle::null_resource, 0, sv._sv);
}

inline void GraphicsPass::add_argument(prebuilt_argument const& sv, const buffer& constant_buffer, uint32_t constant_buffer_offset)
{
    ++mArgNum;
    mCmd.add_shader_arg(constant_buffer.res.handle, constant_buffer_offset, sv._sv);
}

inline void GraphicsPass::add_argument(phi::handle::shader_view sv, phi::handle::resource cbv, uint32_t cbv_offset)
{
    ++mArgNum;
    mCmd.add_shader_arg(cbv, cbv_offset, sv);
}

inline void GraphicsPass::add_argument(const buffer& constant_buffer, uint32_t constant_buffer_offset)
{
    ++mArgNum;
    mCmd.add_shader_arg(constant_buffer.res.handle, constant_buffer_offset);
}
}
