#pragma once

#include <phantasm-hardware-interface/commands.hh>

#include <phantasm-renderer/argument.hh>
#include <phantasm-renderer/fwd.hh>
#include <phantasm-renderer/resource_types.hh>

namespace pr
{
class Frame;

class ComputePipeline
{
public:
    ComputePipeline(Frame* parent, phi::handle::pipeline_state pso);

    void dispatch(unsigned x, unsigned y = 1, unsigned z = 1);

    // shitty WIP API
    void add_argument(baked_argument const& sv);
    void add_argument(baked_argument const& sv, buffer const& constant_buffer, uint32_t constant_buffer_offset = 0);
    void add_argument(buffer const& constant_buffer, uint32_t constant_buffer_offset = 0);

    void pop_argument();
    void clear_arguments();

    template <class T>
    void write_root_constants(T const& data)
    {
        mDispatchCmd.write_root_constants<T>(data);
    }

    ComputePipeline(ComputePipeline&&) noexcept = default;
    ComputePipeline(ComputePipeline const&) = delete;

private:
    Frame* mParent;
    phi::handle::pipeline_state mPSO;
    phi::cmd::dispatch mDispatchCmd;
};
}
