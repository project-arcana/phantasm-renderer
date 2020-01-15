#include "diagnostic_util.hh"

#include <iostream>

#include <renderdoc_app/renderdoc_app.h>

#include <phantasm-renderer/backend/detail/renderdoc_loader.hh>

void pr::backend::vk::util::diagnostic_state::init()
{
    // RenderDoc
    _renderdoc_handle = backend::detail::load_renderdoc();
    if (_renderdoc_handle)
    {
        std::cout << "[pr][backend][vk] RenderDoc detected" << std::endl;
    }
}

void pr::backend::vk::util::diagnostic_state::free()
{
    end_capture();

    if (_renderdoc_handle)
    {
        // anything to do here?
        _renderdoc_handle = nullptr;
    }
}

bool pr::backend::vk::util::diagnostic_state::start_capture()
{
    if (_renderdoc_handle)
    {
        std::cout << "[pr][backend][vk] starting RenderDoc capture" << std::endl;
        _renderdoc_handle->StartFrameCapture(nullptr, nullptr);
        _renderdoc_capture_running = true;
        return true;
    }

    return false;
}

bool pr::backend::vk::util::diagnostic_state::end_capture()
{
    if (_renderdoc_handle && _renderdoc_capture_running)
    {
        std::cout << "[pr][backend][vk] ending RenderDoc capture" << std::endl;
        _renderdoc_handle->EndFrameCapture(nullptr, nullptr);
        _renderdoc_capture_running = false;
        return true;
    }

    return false;
}
