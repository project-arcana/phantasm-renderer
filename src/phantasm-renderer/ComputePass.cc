#include "ComputePass.hh"

#include <clean-core/assert.hh>

#include <phantasm-hardware-interface/Backend.hh>

#include "Frame.hh"

void pr::raii::ComputePass::dispatch(uint32_t x, uint32_t y, uint32_t z)
{
    CC_ASSERT(mCmd.pipeline_state.is_valid() && "PSO is invalid at dispatch submission");

    mCmd.dispatch_x = x;
    mCmd.dispatch_y = y;
    mCmd.dispatch_z = z;

    mParent->flushPendingTransitions();
    mParent->mBackend->cmdDispatch(mParent->mList, mCmd);
}

void pr::raii::ComputePass::dispatch_indirect(buffer const& argument_buffer, uint32_t num_arguments, uint32_t offset_bytes)
{
    CC_ASSERT(mCmd.pipeline_state.is_valid() && "PSO is invalid at dispatch submission");

    phi::cmd::dispatch_indirect dcmd;
    std::memcpy(dcmd.root_constants, mCmd.root_constants, sizeof(dcmd.root_constants));
    std::memcpy(dcmd.shader_arguments.data(), mCmd.shader_arguments.data(), sizeof(dcmd.shader_arguments));
    dcmd.pipeline_state = mCmd.pipeline_state;
    dcmd.argument_buffer_addr.buffer = argument_buffer.handle;
    dcmd.argument_buffer_addr.offset_bytes = offset_bytes;
    dcmd.num_arguments = num_arguments;

    mParent->flushPendingTransitions();
    mParent->mBackend->cmdDispatchIndirect(mParent->mList, dcmd);
}

void pr::raii::ComputePass::add_cached_argument(cc::span<view> srvs,
                                                cc::span<view> uavs,
                                                cc::span<sampler_config const> samplers,
                                                phi::handle::resource constant_buffer,
                                                uint32_t constant_buffer_offset,
                                                uint64_t* pOutHash,
                                                bool* pOutCacheHit)
{
    ++mArgNum;
    mCmd.add_shader_arg(constant_buffer, constant_buffer_offset, mParent->passAcquireShaderView(srvs, uavs, samplers, true, pOutHash, pOutCacheHit));
}
