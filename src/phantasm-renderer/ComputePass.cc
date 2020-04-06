#include "ComputePass.hh"

#include <clean-core/assert.hh>

#include "Frame.hh"

void pr::raii::ComputePass::dispatch(unsigned x, unsigned y, unsigned z)
{
    mCmd.dispatch_x = x;
    mCmd.dispatch_y = y;
    mCmd.dispatch_z = z;

    mParent->passOnDispatch(mCmd);
}


void pr::raii::ComputePass::add_argument(const pr::argument& arg)
{
    ++mArgNum;
    mCmd.add_shader_arg(phi::handle::null_resource, 0, mParent->passAcquireComputeShaderView(arg));
}

void pr::raii::ComputePass::add_argument(const pr::argument& arg, const pr::buffer& constant_buffer, uint32_t constant_buffer_offset)
{
    ++mArgNum;
    mCmd.add_shader_arg(constant_buffer._resource.data.handle, constant_buffer_offset, mParent->passAcquireComputeShaderView(arg));
}
