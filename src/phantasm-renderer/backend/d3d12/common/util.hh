#pragma once

#include <typed-geometry/tg-lean.hh>

#include "d3d12_sanitized.hh"


namespace pr::backend::d3d12::util
{
inline void set_viewport(ID3D12GraphicsCommandList* command_list, tg::ivec2 const& size)
{
    auto const viewport = D3D12_VIEWPORT{0.f, 0.f, float(size.x), float(size.y), 0.f, 1.f};
    auto const scissor_rect = D3D12_RECT{0, 0, LONG(size.x), LONG(size.y)};

    command_list->RSSetViewports(1, &viewport);
    command_list->RSSetScissorRects(1, &scissor_rect);
}

inline void transition_barrier(ID3D12GraphicsCommandList* command_list, ID3D12Resource* res, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
{
    D3D12_RESOURCE_BARRIER barrier;
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = res;
    barrier.Transition.StateBefore = before;
    barrier.Transition.StateAfter = after;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    command_list->ResourceBarrier(1, &barrier);
}
}
