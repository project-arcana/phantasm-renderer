#pragma once

#include "d3d12_fwd.hh"

namespace pr::backend::d3d12::util
{
struct diagnostic_state
{
    void init();
    void free();

    bool start_capture();
    bool end_capture();

private:
    IDXGraphicsAnalysis* _pix_handle = nullptr;
    bool _capture_running = false;
};

void set_pix_marker(ID3D12GraphicsCommandList* cmdlist, UINT64 color, char const* string);
void set_pix_marker(ID3D12CommandQueue* cmdqueue, UINT64 color, char const* string);

void set_pix_marker_cpu(UINT64 color, char const* string);

}
