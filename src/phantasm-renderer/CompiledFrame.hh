#pragma once

#include <clean-core/vector.hh>

#include <phantasm-hardware-interface/handles.hh>

#include <phantasm-renderer/common/state_info.hh>

namespace pr
{
class Context;

/**
 * A CompiledFrame is a compiled command list ready for submission
 */
class CompiledFrame
{
    // move-only type
public:
    CompiledFrame() = default;
    CompiledFrame(CompiledFrame const&) = delete;
    CompiledFrame& operator=(CompiledFrame const&) = delete;
    CompiledFrame(CompiledFrame&& rhs) noexcept
      : parent(rhs.parent), cmdlist(rhs.cmdlist), freeables(cc::move(rhs.freeables)), present_after_submit_swapchain(rhs.present_after_submit_swapchain)
    {
        rhs.parent = nullptr;
    }

    CompiledFrame& operator=(CompiledFrame&& rhs) noexcept
    {
        if (this != &rhs)
        {
            _destroy();
            parent = rhs.parent;
            cmdlist = rhs.cmdlist;
            freeables = cc::move(rhs.freeables);
            present_after_submit_swapchain = rhs.present_after_submit_swapchain;
            rhs.parent = nullptr;
        }

        return *this;
    }

    ~CompiledFrame() { _destroy(); }

private:
    friend class Context;
    CompiledFrame(Context* parent, phi::handle::command_list cmdlist, cc::vector<freeable_cached_obj>&& freeables, phi::handle::swapchain present_after_submit_sc)
      : parent(parent), cmdlist(cmdlist), freeables(cc::move(freeables)), present_after_submit_swapchain(present_after_submit_sc)
    {
    }

    void _destroy();

    Context* parent = nullptr;
    phi::handle::command_list cmdlist = phi::handle::null_command_list;
    cc::vector<freeable_cached_obj> freeables;
    phi::handle::swapchain present_after_submit_swapchain = phi::handle::null_swapchain;
};
}
