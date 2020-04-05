#pragma once

#include <phantasm-hardware-interface/commands.hh>

#include <phantasm-renderer/argument.hh>
#include <phantasm-renderer/fwd.hh>
#include <phantasm-renderer/resource_types.hh>

namespace pr::raii
{
class Frame;

class GraphicsPass
{
public:
    template <class... Args>
    [[nodiscard]] GraphicsPass bind(Args&&... args) &
    {
        GraphicsPass p = {mParent, mCmd, mArgNum};
        p.add_argument(cc::forward<Args>(args)...);
        return p;
    }

    template <class... Args>
    [[nodiscard]] GraphicsPass bind(Args&&... args) &&
    {
        GraphicsPass p = {mParent, mCmd, mArgNum};
        p.add_argument(cc::forward<Args>(args)...);
        return p;
    }

    void draw(unsigned num_vertices);
    void draw(buffer const& vertex_buffer);
    void draw(buffer const& vertex_buffer, buffer const& index_buffer);

    void set_offset(unsigned vertex_offset, unsigned index_offset = 0)
    {
        mCmd.vertex_offset = vertex_offset;
        mCmd.index_offset = index_offset;
    }

    void set_scissor(tg::iaabb2 scissor) { mCmd.scissor = scissor; }
    void set_scissor(int left, int top, int right, int bot) { mCmd.scissor = tg::iaabb2({left, top}, {right, bot}); }
    void unset_scissor() { mCmd.scissor = tg::iaabb2(-1, -1); }

    void set_constant_buffer(buffer const& constant_buffer, unsigned offset = 0);
    void set_constant_buffer_offset(unsigned offset);

    template <class T>
    void write_constants(T const& val)
    {
        mCmd.write_root_constants<T>(val);
    }

    GraphicsPass(GraphicsPass&& rhs) = default;

private:
    friend class Framebuffer;
    // Framebuffer-side ctor
    GraphicsPass(Frame* parent, phi::handle::pipeline_state pso) : mParent(parent) { mCmd.pipeline_state = pso; }

private:
    // internal re-bind ctor
    GraphicsPass(Frame* parent, phi::cmd::draw const& cmd, unsigned arg_i) : mParent(parent), mCmd(cmd), mArgNum(arg_i) {}

private:
    void add_argument(argument const& arg);
    void add_argument(argument const& arg, buffer const& constant_buffer, uint32_t constant_buffer_offset = 0);

    void add_argument(prebuilt_argument const& sv);
    void add_argument(prebuilt_argument const& sv, buffer const& constant_buffer, uint32_t constant_buffer_offset = 0);

    void add_argument(buffer const& constant_buffer, uint32_t constant_buffer_offset = 0);

    Frame* mParent = nullptr;
    phi::cmd::draw mCmd;
    // index of owning argument - 1, 0 means no arguments existing
    unsigned mArgNum = 0;
};
}
