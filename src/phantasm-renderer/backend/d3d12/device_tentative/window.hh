#pragma once
#ifdef PR_BACKEND_D3D12

// Win32 HWND forward declaration
typedef struct HWND__* HWND;

namespace pr::backend::d3d12
{
/// Tentative precursor of eventual device-abstraction library
/// Only one allowed at the time
class Window
{
public:
    struct event_proxy;

    // reference type
public:
    Window() = default;
    Window(Window const&) = delete;
    Window(Window&&) noexcept = delete;
    Window& operator=(Window const&) = delete;
    Window& operator=(Window&&) noexcept = delete;
    ~Window();

    void initialize(char const* title);

    void pollEvents();

    [[nodiscard]] HWND getHandle() const { return mHandle; }
    [[nodiscard]] bool isRequestingClose() const { return mIsRequestingClose; }

private:
    void onCloseEvent();

    HWND mHandle;
    bool mIsRequestingClose = false;
};

}

#endif
