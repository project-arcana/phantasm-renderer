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
    template <class... Args>
    [[nodiscard]] ComputePass bind(Args&&... args)
    {
        ComputePass p = {mParent, mCmd, mArgNum};
        p.add_argument(cc::forward<Args>(args)...);
        return p;
    }

    void dispatch(unsigned x, unsigned y = 1, unsigned z = 1);

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
    void add_argument(argument const& arg);
    void add_argument(argument const& arg, buffer const& constant_buffer, uint32_t constant_buffer_offset = 0);

    void add_argument(prebuilt_argument const& sv);
    void add_argument(prebuilt_argument const& sv, buffer const& constant_buffer, uint32_t constant_buffer_offset = 0);
    void add_argument(buffer const& constant_buffer, uint32_t constant_buffer_offset = 0);

    // raw phi
    void add_argument(phi::handle::shader_view sv, phi::handle::resource cbv = phi::handle::null_resource, uint32_t cbv_offset = 0);

    Frame* mParent = nullptr;
    phi::cmd::dispatch mCmd;
    // index of owning argument - 1, 0 means no arguments existing
    unsigned mArgNum = 0;
};

// inline implementation

inline void ComputePass::set_constant_buffer(const buffer& constant_buffer, unsigned offset)
{
    set_constant_buffer(constant_buffer.res.handle, offset);
}

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

inline void ComputePass::add_argument(prebuilt_argument const& sv)
{
    ++mArgNum;
    mCmd.add_shader_arg(phi::handle::null_resource, 0, sv._sv);
}

inline void ComputePass::add_argument(prebuilt_argument const& sv, const buffer& constant_buffer, uint32_t constant_buffer_offset)
{
    ++mArgNum;
    mCmd.add_shader_arg(constant_buffer.res.handle, constant_buffer_offset, sv._sv);
}

inline void ComputePass::add_argument(const buffer& constant_buffer, uint32_t constant_buffer_offset)
{
    ++mArgNum;
    mCmd.add_shader_arg(constant_buffer.res.handle, constant_buffer_offset);
}

inline void ComputePass::add_argument(phi::handle::shader_view sv, phi::handle::resource cbv, uint32_t cbv_offset)
{
    ++mArgNum;
    mCmd.add_shader_arg(cbv, cbv_offset, sv);
}
}
