#include "diagnostic_util.hh"

// clang-format off
#include "d3d12_sanitized.hh"
#include <DXProgrammableCapture.h>
#include <WinPixEventRuntime/pix3.h>
// clang-format on

#include <renderdoc_app/renderdoc_app.h>

#include <phantasm-renderer/backend/detail/renderdoc_loader.hh>
#include <phantasm-renderer/backend/d3d12/common/log.hh>

#include "verify.hh"

void pr::backend::d3d12::util::diagnostic_state::init()
{
    // PIX
    if (detail::hr_succeeded(::DXGIGetDebugInterface1(0, IID_PPV_ARGS(&_pix_handle))))
    {
        _pix_capture_running = false;
        log::info() << "PIX detected";
    }
    else
    {
        _pix_handle = nullptr;
    }

    // RenderDoc
    _renderdoc_handle = backend::detail::load_renderdoc();
    if (_renderdoc_handle)
    {
        log::info() << "RenderDoc detected";
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

    if (_renderdoc_handle)
    {
        // anything to do here?
        _renderdoc_handle = nullptr;
    }
}

bool pr::backend::d3d12::util::diagnostic_state::start_capture()
{
    if (_pix_handle)
    {
        log::info() << "starting PIX capture";
        _pix_handle->BeginCapture();
        _pix_capture_running = true;
        return true;
    }
    else if (_renderdoc_handle)
    {
        log::info() << "starting RenderDoc capture";
        _renderdoc_handle->StartFrameCapture(nullptr, nullptr);
        _renderdoc_capture_running = true;
        return true;
    }

    return false;
}

bool pr::backend::d3d12::util::diagnostic_state::end_capture()
{
    if (_pix_handle && _pix_capture_running)
    {
        log::info() << "ending PIX capture";
        _pix_handle->EndCapture();
        _pix_capture_running = false;
        return true;
    }
    else if (_renderdoc_handle && _renderdoc_capture_running)
    {
        log::info() << "ending RenderDoc capture";
        _renderdoc_handle->EndFrameCapture(nullptr, nullptr);
        _renderdoc_capture_running = false;
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
