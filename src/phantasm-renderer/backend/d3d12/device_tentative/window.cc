#include "window.hh"
#ifdef PR_BACKEND_D3D12

#include <clean-core/assert.hh>
#include <clean-core/native/win32_sanitized.hh>

struct pr::backend::d3d12::Window::event_proxy
{
    pr::backend::d3d12::Window* window = nullptr;
    void delegateEventOnClose() const { window->onCloseEvent(); }
};

namespace
{
pr::backend::d3d12::Window::event_proxy sEventProxy;

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LRESULT res = 0;
    switch (msg)
    {
    case WM_QUIT:
    case WM_CLOSE:
        sEventProxy.delegateEventOnClose();
        break;
    default:
        res = DefWindowProc(hWnd, msg, wParam, lParam);
        break;
    }
    return res;
}

}

pr::backend::d3d12::Window::~Window()
{
    CC_ASSERT(sEventProxy.window == this);
    sEventProxy.window = nullptr;
}

void pr::backend::d3d12::Window::initialize(const char* title)
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

void pr::backend::d3d12::Window::pollEvents()
{
    MSG msg = {};
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void pr::backend::d3d12::Window::onCloseEvent() { mIsRequestingClose = true; }

#endif
