#pragma once

#include <phantasm-hardware-interface/commands.hh>

#include <phantasm-renderer/argument.hh>
#include <phantasm-renderer/fwd.hh>
#include <phantasm-renderer/resource_types.hh>

namespace pr
{
class Frame;

class ComputePass
{
public:
    template <class... Args>
    [[nodiscard]] ComputePass bind(Args&&... args) &
    {
        ComputePass p = {mParent, mCmd, mArgNum};
        p.add_argument(cc::forward<Args>(args)...);
        return p;
    }

    template <class... Args>
    [[nodiscard]] ComputePass bind(Args&&... args) &&
    {
        ComputePass p = {mParent, mCmd, mArgNum};
        p.add_argument(cc::forward<Args>(args)...);
        return p;
    }

    void dispatch(unsigned x, unsigned y = 1, unsigned z = 1);

    void set_constant_buffer(buffer const& constant_buffer, unsigned offset = 0);
    void set_constant_buffer_offset(unsigned offset);

    template <class T>
    void write_root_constants(T const& val)
    {
        mCmd.write_root_constants<T>(val);
    }

    ComputePass(ComputePass&& rhs) = default;

private:
    friend class Frame;
    // Frame-side ctor
    ComputePass(Frame* parent, phi::handle::pipeline_state pso) : mParent(parent) { mCmd.pipeline_state = pso; }

private:
    // internal re-bind ctor
    ComputePass(Frame* parent, phi::cmd::dispatch const& cmd, unsigned arg_i) : mParent(parent), mCmd(cmd), mArgNum(arg_i) {}

private:
    void add_argument(argument const& arg);
    void add_argument(argument const& arg, buffer const& constant_buffer, uint32_t constant_buffer_offset = 0);

    void add_argument(baked_argument const& sv);
    void add_argument(baked_argument const& sv, buffer const& constant_buffer, uint32_t constant_buffer_offset = 0);
    void add_argument(buffer const& constant_buffer, uint32_t constant_buffer_offset = 0);

    Frame* mParent = nullptr;
    phi::cmd::dispatch mCmd;
    // index of owning argument - 1, 0 means no arguments existing
    unsigned mArgNum = 0;
};
}
