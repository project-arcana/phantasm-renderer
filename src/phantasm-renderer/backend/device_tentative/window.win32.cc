#include "window.hh"

#ifdef CC_OS_WINDOWS
#include <clean-core/assert.hh>
#include <clean-core/native/win32_sanitized.hh>

#ifdef PR_BACKEND_VULKAN
#include <phantasm-renderer/backend/vulkan/common/zero_struct.hh>
#include <phantasm-renderer/backend/vulkan/common/verify.hh>
#endif

struct pr::backend::device::Window::event_proxy
{
    pr::backend::device::Window* window = nullptr;
    void delegateEventOnClose() const { window->onCloseEvent(); }
    void delegateEventResize(int w, int h, bool minimized) const { window->onResizeEvent(w, h, minimized); }
};

namespace
{
pr::backend::device::Window::event_proxy sEventProxy;

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_QUIT:
    case WM_CLOSE:
        sEventProxy.delegateEventOnClose();
        return 0;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    case WM_SIZE:
    {
        RECT client_rect;
        ::GetClientRect(hWnd, &client_rect);
        sEventProxy.delegateEventResize(client_rect.right - client_rect.left, client_rect.bottom - client_rect.top, (::IsIconic(hWnd) == TRUE));
        return 0;
    }
    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
}

}

pr::backend::device::Window::~Window()
{
    CC_ASSERT(sEventProxy.window == this);
    ::DestroyWindow(mHandle);
    sEventProxy.window = nullptr;
}

void pr::backend::device::Window::initialize(const char* title)
{
    // No two windows allowed at the same time
    CC_ASSERT(sEventProxy.window == nullptr);
    sEventProxy.window = this;

    auto hInstance = GetModuleHandle(nullptr);

    // Window Setup
    WNDCLASS windowClass = {};
    windowClass.lpfnWndProc = WndProc;
    windowClass.hInstance = hInstance;
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    windowClass.lpszClassName = "MiniWindow";
    RegisterClass(&windowClass);

    mHandle = CreateWindowEx(0, windowClass.lpszClassName, title, WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                             CW_USEDEFAULT, nullptr, nullptr, hInstance, nullptr);
}

void pr::backend::device::Window::pollEvents()
{
    MSG msg = {};
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

cc::vector<char const*> pr::backend::device::Window::getRequiredInstanceExtensions()
{
    cc::vector<char const*> res;
    res.reserve(5);

    res.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
    res.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);

    // if linux: VK_KHR_XCB_SURFACE_EXTENSION_NAME

    return res;
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
void pr::backend::device::Window::createVulkanSurface(VkInstance instance, VkSurfaceKHR &out_surface)
{
    VkWin32SurfaceCreateInfoKHR surface_info;
    vk::zero_info_struct(surface_info, VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR);
    surface_info.hwnd = mHandle;
    surface_info.hinstance = GetModuleHandle(nullptr);
    PR_VK_VERIFY_SUCCESS(vkCreateWin32SurfaceKHR(instance, &surface_info, nullptr, &out_surface));
}
#endif

#endif
