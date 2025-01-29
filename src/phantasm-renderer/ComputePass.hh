#pragma once

#include <phantasm-hardware-interface/commands.hh>

#include <phantasm-renderer/argument.hh>
#include <phantasm-renderer/common/api.hh>
#include <phantasm-renderer/fwd.hh>
#include <phantasm-renderer/resource_types.hh>

namespace pr::raii
{
class Frame;

class PR_API ComputePass
{
public:
    [[nodiscard]] ComputePass bind(prebuilt_argument sv, buffer constant_buffer = {}, uint32_t constant_buffer_offset = 0)
    {
        return bind(sv._sv, constant_buffer.handle, constant_buffer_offset);
    }

    [[nodiscard]] ComputePass bind(prebuilt_argument sv, phi::buffer_address constant_buffer)
    {
        return bind(sv._sv, constant_buffer.buffer, constant_buffer.offset_bytes);
    }

    // CBV only
    [[nodiscard]] ComputePass bind(buffer constant_buffer, uint32_t constant_buffer_offset = 0)
    {
        return bind(phi::handle::null_shader_view, constant_buffer.handle, constant_buffer_offset);
    }

    [[nodiscard]] ComputePass bind(phi::buffer_address constant_buffer)
    {
        return bind(phi::handle::null_shader_view, constant_buffer.buffer, constant_buffer.offset_bytes);
    }

    // raw phi
    [[nodiscard]] ComputePass bind(phi::handle::shader_view sv, phi::handle::resource cbv = {}, uint32_t cbv_offset = 0)
    {
        ComputePass p = {mParent, mCmd, mArgNum};
        p.add_argument(sv, cbv, cbv_offset);
        return p;
    }

    [[nodiscard]] ComputePass bind(phi::handle::shader_view sv, phi::buffer_address cbv)
    {
        ComputePass p = {mParent, mCmd, mArgNum};
        p.add_argument(sv, cbv.buffer, cbv.offset_bytes);
        return p;
    }

    // cache-access variants
    // hits a OS mutex
    [[nodiscard]] ComputePass bind(argument& arg, phi::buffer_address cbv = {})
    {
        return bind(arg.srvs, arg.uavs, arg.samplers, {cbv.buffer}, cbv.offset_bytes);
    }

    [[nodiscard]] ComputePass bind(argument& arg, buffer constant_buffer, uint32_t constant_buffer_offset = 0)
    {
        return bind(arg.srvs, arg.uavs, arg.samplers, constant_buffer, constant_buffer_offset);
    }

    [[nodiscard]] ComputePass bind(cc::span<view> srvs, cc::span<view> uavs, cc::span<sampler_config const> samplers, buffer constant_buffer, uint32_t constant_buffer_offset = 0)
    {
        ComputePass p = {mParent, mCmd, mArgNum};
        p.add_cached_argument(srvs, uavs, samplers, constant_buffer.handle, constant_buffer_offset, nullptr, nullptr);
        return p;
    }

    void dispatch(uint32_t x, uint32_t y = 1, uint32_t z = 1);

    // dispatch for a given work size
    // example: dispatch2D(4096, 64);
    void dispatch1D(uint32_t sizeX, uint32_t groupSizeX = 64) { dispatch(cc::int_div_ceil(sizeX, groupSizeX), 1u, 1u); }

    // dispatch for a given work size
    // example: dispatch2D(1920, 1080, 8, 8);
    void dispatch2D(uint32_t sizeX, uint32_t sizeY, uint32_t groupSizeX = 8, uint32_t groupSizeY = 8)
    {
        dispatch(cc::int_div_ceil(sizeX, groupSizeX), cc::int_div_ceil(sizeY, groupSizeY), 1u);
    }

    // dispatch for a given work size
    // example: dispatch2D(512, 512, 512, 8, 8, 8);
    void dispatch3D(uint32_t sizeX, uint32_t sizeY, uint32_t sizeZ, uint32_t groupSizeX = 8, uint32_t groupSizeY = 8, uint32_t groupSizeZ = 8)
    {
        dispatch(cc::int_div_ceil(sizeX, groupSizeX), cc::int_div_ceil(sizeY, groupSizeY), cc::int_div_ceil(sizeZ, groupSizeZ));
    }

    void dispatch_indirect(buffer const& argument_buffer, uint32_t num_arguments = 1, uint32_t offset_bytes = 0);

    void set_constant_buffer(buffer const& constant_buffer, unsigned offset = 0);
    void set_constant_buffer(phi::handle::resource raw_cbv, unsigned offset = 0);
    void set_constant_buffer_offset(unsigned offset);

    template <class T>
    void write_constants(T const& val)
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
    // persisted, raw phi
    void add_argument(phi::handle::shader_view sv, phi::handle::resource cbv, uint32_t cbv_offset);

    // cache-access variant
    // hits a OS mutex
    void add_cached_argument(cc::span<view> srvs,
                             cc::span<view> uavs,
                             cc::span<sampler_config const> samplers,
                             phi::handle::resource cbv,
                             uint32_t cbv_offset,
                             uint64_t* pOutHash,
                             bool* pOutCacheHit);

    Frame* mParent = nullptr;
    phi::cmd::dispatch mCmd;
    // index of owning argument - 1, 0 means no arguments existing
    unsigned mArgNum = 0;
};

// inline implementation

inline void ComputePass::set_constant_buffer(const buffer& constant_buffer, unsigned offset) { set_constant_buffer(constant_buffer.handle, offset); }

inline void ComputePass::set_constant_buffer(phi::handle::resource raw_cbv, unsigned offset)
{
    CC_ASSERT(mArgNum != 0 && "Attempted to set_constant_buffer on a ComputePass without prior bind");
    mCmd.shader_arguments[uint8_t(mArgNum - 1)].constant_buffer = raw_cbv;
    mCmd.shader_arguments[uint8_t(mArgNum - 1)].constant_buffer_offset = offset;
}

inline void ComputePass::set_constant_buffer_offset(unsigned offset)
{
    CC_ASSERT(mArgNum != 0 && "Attempted to set_constant_buffer_offset on a ComputePass without prior bind");
    mCmd.shader_arguments[uint8_t(mArgNum - 1)].constant_buffer_offset = offset;
}

inline void ComputePass::add_argument(phi::handle::shader_view sv, phi::handle::resource cbv, uint32_t cbv_offset)
{
    ++mArgNum;
    mCmd.add_shader_arg(cbv, cbv_offset, sv);
}
}
