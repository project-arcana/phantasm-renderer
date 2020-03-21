#include "ComputePass.hh"

#include <clean-core/assert.hh>

#include "Frame.hh"

pr::ComputePass::ComputePass(pr::Frame* parent, phi::handle::pipeline_state pso) : mParent(parent), mPSO(pso) { mDispatchCmd.pipeline_state = mPSO; }

void pr::ComputePass::dispatch(unsigned x, unsigned y, unsigned z)
{
    mDispatchCmd.dispatch_x = x;
    mDispatchCmd.dispatch_y = y;
    mDispatchCmd.dispatch_z = z;

    mParent->pipelineOnDispatch(mDispatchCmd);
}

void pr::ComputePass::add_argument(const pr::baked_argument& sv) { mDispatchCmd.add_shader_arg(phi::handle::null_resource, 0, sv.data._sv); }

void pr::ComputePass::add_argument(const pr::baked_argument& sv, const pr::buffer& constant_buffer, uint32_t constant_buffer_offset)
{
    mDispatchCmd.add_shader_arg(constant_buffer._resource.data.handle, constant_buffer_offset, sv.data._sv);
}

void pr::ComputePass::add_argument(const pr::buffer& constant_buffer, uint32_t constant_buffer_offset)
{
    mDispatchCmd.add_shader_arg(constant_buffer._resource.data.handle, constant_buffer_offset);
}

void pr::ComputePass::pop_argument() { mDispatchCmd.shader_arguments.pop_back(); }

void pr::ComputePass::clear_arguments() { mDispatchCmd.shader_arguments.clear(); }
