#include "window.hh"

#include <clean-core/assert.hh>
#include <clean-core/native/win32_sanitized.hh>

struct pr::backend::d3d12::Window::event_proxy
{
    pr::backend::d3d12::Window* window = nullptr;
    void delegateEventOnClose() const { window->onCloseEvent(); }
    void delegateEventResize(int w, int h, bool minimized) const { window->onResizeEvent(w, h, minimized); }
};

namespace
{
pr::backend::d3d12::Window::event_proxy sEventProxy;

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

pr::backend::d3d12::Window::~Window()
{
    CC_ASSERT(sEventProxy.window == this);
    ::DestroyWindow(mHandle);
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

void pr::backend::d3d12::Window::onResizeEvent(int w, int h, bool minimized)
{
    mWidth = w;
    mHeight = h;
    mIsMinimized = minimized;
    mPendingResize = true;
}

