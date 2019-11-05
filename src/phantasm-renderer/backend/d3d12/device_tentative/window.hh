#pragma once

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

    /// Whether a close event has been fired
    [[nodiscard]] bool isRequestingClose() const { return mIsRequestingClose; }

    /// Clears pending resizes
    void clearPendingResize() { mPendingResize = false; }

    /// Whether a resize happened since the last call to clearPendingResize
    [[nodiscard]] bool isPendingResize() const { return mPendingResize; }

    [[nodiscard]] int getWidth() const { return mWidth; }
    [[nodiscard]] int getHeight() const { return mHeight; }
    [[nodiscard]] bool isMinimized() const { return mIsMinimized; }

private:
    void onCloseEvent();
    void onResizeEvent(int w, int h, bool minimized);

    HWND mHandle;
    int mWidth = 0;
    int mHeight = 0;
    bool mIsMinimized = false;
    bool mPendingResize = false;
    bool mIsRequestingClose = false;
};

}

