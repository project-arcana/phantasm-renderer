#include "surface_util.hh"

#include <clean-core/array.hh>
#include <clean-core/macros.hh>
#include <clean-core/bit_cast.hh>

#include "common/verify.hh"
#include "loader/volk.hh"

#ifdef CC_OS_WINDOWS
#include <clean-core/native/win32_sanitized.hh>
#elif defined(CC_OS_LINUX)
#include <X11/Xlib.h>
#endif

#ifdef PR_BACKEND_HAS_SDL2
#include <SDL2/SDL_vulkan.h>
#endif

#ifdef PR_BACKEND_VULKAN
namespace
{
#ifdef CC_OS_WINDOWS
constexpr cc::array<char const*, 2> gc_required_vulkan_extensions = {VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME};
#elif defined(CC_OS_LINUX)
constexpr cc::array<char const*, 2> gc_required_vulkan_extensions = {VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_XLIB_SURFACE_EXTENSION_NAME};
#endif

}
#endif

VkSurfaceKHR pr::backend::vk::create_platform_surface(VkInstance instance, const pr::backend::native_window_handle& window_handle)
{
    VkSurfaceKHR res_surface = nullptr;

#ifdef CC_OS_WINDOWS
    VkWin32SurfaceCreateInfoKHR surface_info = {};
    surface_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surface_info.hwnd = static_cast<::HWND>(window_handle.native_a);
    surface_info.hinstance = GetModuleHandle(nullptr);
    PR_VK_VERIFY_SUCCESS(vkCreateWin32SurfaceKHR(instance, &surface_info, nullptr, &res_surface));
#elif defined(CC_OS_LINUX)

    if (window_handle.native_a && window_handle.native_b)
    {
        // XLib mode
        VkXlibSurfaceCreateInfoKHR surface_info = {};
        surface_info.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
        surface_info.dpy = static_cast<::Display*>(window_handle.native_b);
        surface_info.window = cc::bit_cast<::Window>(window_handle.native_a);
        surface_info.pNext = nullptr;
        surface_info.flags = 0;
        PR_VK_VERIFY_SUCCESS(vkCreateXlibSurfaceKHR(instance, &surface_info, nullptr, &res_surface));
    }
    else if (window_handle.native_a && !window_handle.native_b)
    {
        // SDL2 mode
#ifdef PR_BACKEND_HAS_SDL2
        SDL_Vulkan_CreateSurface(static_cast<::SDL_Window*>(window_handle.native_a), instance, &res_surface);
#else
        CC_RUNTIME_ASSERT(false && "SDL support disabled, window_handle invalid");
#endif
    }
    else
    {
        CC_RUNTIME_ASSERT(false && "window_handle invalid");
    }

#else
#error "Unimplemented platform"
#endif

    return res_surface;
}

cc::span<const char* const> pr::backend::vk::get_platform_instance_extensions() { return gc_required_vulkan_extensions; }
