#pragma once

#ifdef PR_BACKEND_VULKAN
#include <clean-core/span.hh>

typedef struct VkInstance_T* VkInstance;
typedef struct VkSurfaceKHR_T* VkSurfaceKHR;
#endif

#ifdef PR_BACKEND_D3D12
typedef struct HWND__* HWND;
#endif

namespace pr::backend::device
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

    void initialize(char const* title, int width = 850, int height = 550);

    void pollEvents();


    /// Whether a close event has been fired
    [[nodiscard]] bool isRequestingClose() const { return mIsRequestingClose; }

    /// Clears pending resizes
    void clearPendingResize() { mPendingResize = false; }

    /// Whether a resize happened since the last call to clearPendingResize
    [[nodiscard]] bool isPendingResize() const { return mPendingResize; }

    [[nodiscard]] int getWidth() const { return mWidth; }
    [[nodiscard]] int getHeight() const { return mHeight; }
    [[nodiscard]] bool isMinimized() const { return mIsMinimized; }

public:
#ifdef PR_BACKEND_VULKAN
    static cc::span<char const*> getRequiredInstanceExtensions();
    void createVulkanSurface(VkInstance instance, VkSurfaceKHR& out_surface);
#endif

#ifdef PR_BACKEND_D3D12
    [[nodiscard]] ::HWND getHandle() const;
#endif

private:
    void onCloseEvent();
    void onResizeEvent(int w, int h, bool minimized);

    int mWidth = 0;
    int mHeight = 0;
    bool mIsMinimized = false;
    bool mPendingResize = false;
    bool mIsRequestingClose = false;
};

}
