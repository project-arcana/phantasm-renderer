#include <clean-core/macros.hh>

#include "window.hh"

#ifdef CC_OS_WINDOWS

#include <clean-core/array.hh>
#include <clean-core/assert.hh>

// clang-format off
#include <clean-core/native/detail/win32_sanitize_before.inl>

#include <Windows.h>
#include <ShellScalingApi.h>

#include <clean-core/native/detail/win32_sanitize_after.inl>
// clang-format on

#ifdef PR_BACKEND_VULKAN
#include <phantasm-renderer/backend/vulkan/common/verify.hh>
#include <phantasm-renderer/backend/vulkan/loader/volk.hh>
#endif

struct pr::backend::device::Window::event_proxy
{
    pr::backend::device::Window* window = nullptr;
    void delegateEventOnClose() const { window->onCloseEvent(); }
    void delegateEventResize(int w, int h, bool minimized) const { window->onResizeEvent(w, h, minimized); }
};

namespace
{
pr::backend::device::Window::event_proxy s_event_proxy;
::HWND s_window_handle;

::LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_QUIT:
    case WM_CLOSE:
        s_event_proxy.delegateEventOnClose();
        return 0;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    case WM_SIZE:
    {
        RECT client_rect;
        ::GetClientRect(hWnd, &client_rect);
        s_event_proxy.delegateEventResize(client_rect.right - client_rect.left, client_rect.bottom - client_rect.top, (::IsIconic(hWnd) == TRUE));
        return 0;
    }
    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
}
}

pr::backend::device::Window::~Window()
{
    CC_ASSERT(s_event_proxy.window == this);
    ::DestroyWindow(s_window_handle);
    s_event_proxy.window = nullptr;
}

void pr::backend::device::Window::initialize(const char* title, int width, int height)
{
    // No two windows allowed at the same time
    CC_ASSERT(s_event_proxy.window == nullptr);
    s_event_proxy.window = this;

    // Set process DPI awareness
    {
        // PER_MONITOR_AWARE: No scaling by Windows, no lies by GetClientRect
        // Possible alternative is PER_MONITOR_AWARE_V2, with better per-window DPI access,
        // but it requires the Win10 Creators update.
        // While this setting is definitively a good idea, support for HighDPI should
        // possibly play a role in the future da::Window
        ::SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);
    }

    // Retrieve primary monitor scale factor and adapt width/height accordingly
    {
        POINT const zero_point = {0, 0};
        auto const primary_monitor = ::MonitorFromPoint(zero_point, MONITOR_DEFAULTTOPRIMARY);

        DEVICE_SCALE_FACTOR scale_factor;
        auto const hres = ::GetScaleFactorForMonitor(primary_monitor, &scale_factor);

        if (hres == S_OK && scale_factor != DEVICE_SCALE_FACTOR_INVALID)
        {
            mScaleFactor = int(scale_factor) / 100.f;
            width = int(width * mScaleFactor);
            height = int(height * mScaleFactor);
        }
    }

    mWidth = width;
    mHeight = height;

    auto const hInstance = GetModuleHandle(nullptr);

    // Window Setup
    WNDCLASS windowClass = {};
    windowClass.lpfnWndProc = WndProc;
    windowClass.hInstance = hInstance;
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    windowClass.lpszClassName = "MiniWindow";
    RegisterClass(&windowClass);

    s_window_handle = CreateWindowEx(0, windowClass.lpszClassName, title, WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, mWidth,
                                     mHeight, nullptr, nullptr, hInstance, nullptr);
}

void pr::backend::device::Window::pollEvents()
{
    ::MSG msg = {};
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        ::TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void pr::backend::device::Window::onCloseEvent() { mIsRequestingClose = true; }

void pr::backend::device::Window::onResizeEvent(int w, int h, bool minimized)
{
    mWidth = w;
    mHeight = h;
    mIsMinimized = minimized;
    mPendingResize = true;
}

#ifdef PR_BACKEND_VULKAN
namespace
{
constexpr cc::array<char const*, 2> s_required_vulkan_extensions = {VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME};
}

cc::span<char const* const> pr::backend::device::Window::getRequiredInstanceExtensions() { return s_required_vulkan_extensions; }

void pr::backend::device::Window::createVulkanSurface(VkInstance instance, VkSurfaceKHR& out_surface)
{
    VkWin32SurfaceCreateInfoKHR surface_info = {};
    surface_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surface_info.hwnd = s_window_handle;
    surface_info.hinstance = GetModuleHandle(nullptr);
    PR_VK_VERIFY_SUCCESS(vkCreateWin32SurfaceKHR(instance, &surface_info, nullptr, &out_surface));
}
#endif // PR_BACKEND_VULKAN

#ifdef PR_BACKEND_D3D12
HWND pr::backend::device::Window::getHandle() const { return s_window_handle; }
#endif // PR_BACKEND_D3D12

#endif // CC_OS_WINDOWS
