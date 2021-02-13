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

void pr::raii::ComputePass::add_cached_argument(const pr::argument& arg, phi::handle::resource constant_buffer, uint32_t constant_buffer_offset)
{
    ++mArgNum;
    mCmd.add_shader_arg(constant_buffer, constant_buffer_offset, mParent->passAcquireComputeShaderView(arg));
}
