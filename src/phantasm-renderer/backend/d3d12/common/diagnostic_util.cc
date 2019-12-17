#include "diagnostic_util.hh"

#include <cstdarg>
#include <iostream>

// clang-format off
#include "d3d12_sanitized.hh"
#include <DXProgrammableCapture.h>
#include <WinPixEventRuntime/pix3.h>
// clang-format on

#include "verify.hh"

void pr::backend::d3d12::util::diagnostic_state::init()
{
    if (detail::hr_succeeded(::DXGIGetDebugInterface1(0, IID_PPV_ARGS(&_pix_handle))))
    {
        _capture_running = false;
        std::cout << "[pr][backend][d3d12] PIX detected" << std::endl;
    }
    else
    {
        _pix_handle = nullptr;
    }
}

void pr::backend::d3d12::util::diagnostic_state::free()
{
    end_capture();

    if (_pix_handle)
    {
        _pix_handle->Release();
        _pix_handle = nullptr;
    }
}

bool pr::backend::d3d12::util::diagnostic_state::start_capture()
{
    if (_pix_handle)
    {
        std::cout << "[pr][backend][d3d12] starting PIX capture" << std::endl;
        _pix_handle->BeginCapture();
        _capture_running = true;
        return true;
    }

    return false;
}

bool pr::backend::d3d12::util::diagnostic_state::end_capture()
{
    if (_pix_handle && _capture_running)
    {
        std::cout << "[pr][backend][d3d12] ending PIX capture" << std::endl;
        _pix_handle->EndCapture();
        _capture_running = false;
        return true;
    }

    return false;
}

void pr::backend::d3d12::util::set_pix_marker(ID3D12GraphicsCommandList* cmdlist, UINT64 color, const char* string)
{
    ::PIXSetMarker(cmdlist, color, string);
}

void pr::backend::d3d12::util::set_pix_marker(ID3D12CommandQueue* cmdqueue, UINT64 color, const char* string)
{
    ::PIXSetMarker(cmdqueue, color, string);
}

void pr::backend::d3d12::util::set_pix_marker_cpu(UINT64 color, const char* string) { ::PIXSetMarker(color, string); }
