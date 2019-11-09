#pragma once

#include <typed-geometry/tg-lean.hh>

#include "d3d12_sanitized.hh"


namespace pr::backend::d3d12::util {

inline void set_viewport(ID3D12GraphicsCommandList* command_list, tg::ivec2 const& size)
{
    auto const viewport = D3D12_VIEWPORT{0.f, 0.f, float(size.x), float(size.y), 0.f, 1.f};
    auto const scissor_rect = D3D12_RECT{0, 0, LONG(size.x), LONG(size.y)};

    command_list->RSSetViewports(1, &viewport);
    command_list->RSSetScissorRects(1, &scissor_rect);
}

}
