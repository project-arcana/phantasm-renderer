#include "ComputePass.hh"

#include <clean-core/assert.hh>

#include "Frame.hh"

void pr::ComputePass::dispatch(unsigned x, unsigned y, unsigned z)
{
    mCmd.dispatch_x = x;
    mCmd.dispatch_y = y;
    mCmd.dispatch_z = z;

    mParent->passOnDispatch(mCmd);
}

void pr::ComputePass::set_constant_buffer(const pr::buffer& constant_buffer, unsigned offset)
{
    CC_ASSERT(mArgNum != 0 && "Attempted to set_constant_buffer on a ComputePass without prior bind");
    mCmd.shader_arguments[uint8_t(mArgNum - 1)].constant_buffer = constant_buffer._resource.data.handle;
    mCmd.shader_arguments[uint8_t(mArgNum - 1)].constant_buffer_offset = offset;
}

void pr::ComputePass::set_constant_buffer_offset(unsigned offset)
{
    CC_ASSERT(mArgNum != 0 && "Attempted to set_constant_buffer_offset on a ComputePass without prior bind");
    mCmd.shader_arguments[uint8_t(mArgNum - 1)].constant_buffer_offset = offset;
}

void pr::ComputePass::add_argument(const pr::argument& arg)
{
    ++mArgNum;
    mCmd.add_shader_arg(phi::handle::null_resource, 0, mParent->passAcquireComputeShaderView(arg));
}

void pr::ComputePass::add_argument(const pr::argument& arg, const pr::buffer& constant_buffer, uint32_t constant_buffer_offset)
{
    ++mArgNum;
    mCmd.add_shader_arg(constant_buffer._resource.data.handle, constant_buffer_offset, mParent->passAcquireComputeShaderView(arg));
}

void pr::ComputePass::add_argument(const pr::baked_argument& sv)
{
    ++mArgNum;
    mCmd.add_shader_arg(phi::handle::null_resource, 0, sv.data._sv);
}

void pr::ComputePass::add_argument(const pr::baked_argument& sv, const pr::buffer& constant_buffer, uint32_t constant_buffer_offset)
{
    ++mArgNum;
    mCmd.add_shader_arg(constant_buffer._resource.data.handle, constant_buffer_offset, sv.data._sv);
}

void pr::ComputePass::add_argument(const pr::buffer& constant_buffer, uint32_t constant_buffer_offset)
{
    ++mArgNum;
    mCmd.add_shader_arg(constant_buffer._resource.data.handle, constant_buffer_offset);
}
