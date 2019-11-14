#include "window.hh"
#ifdef CC_OS_LINUX

#include <iostream>

#include <X11/Xlib.h>

#ifdef PR_BACKEND_VULKAN
#include <phantasm-renderer/backend/vulkan/common/verify.hh>
#include <phantasm-renderer/backend/vulkan/common/zero_struct.hh>
#endif

namespace
{
::Display* s_display = nullptr;
::Window s_window;
int s_screen;

::Atom s_atom_delete_message;
::Atom s_atom_minimized;
::Atom s_atom_unminimized;
::Atom s_atom_maximized_h;
::Atom s_atom_maximized_v;
}

pr::backend::device::Window::~Window()
{
    CC_RUNTIME_ASSERT(s_display != nullptr);

    // A requested close is triggered by a WM_DELETE_WINDOW message, which already deletes everything
    if (!mIsRequestingClose)
        ::XCloseDisplay(s_display);

    s_display = nullptr;
}

void pr::backend::device::Window::initialize(const char* title)
{
    CC_RUNTIME_ASSERT(s_display == nullptr && "Only one window allowed");
    s_display = ::XOpenDisplay(nullptr);
    CC_RUNTIME_ASSERT(s_display != nullptr && "Failed to open display");

    s_screen = DefaultScreen(s_display);

    s_window = XCreateSimpleWindow(s_display, RootWindow(s_display, s_screen), 10, 10, 1080, 720, 1, BlackPixel(s_display, s_screen),
                                   WhitePixel(s_display, s_screen));

    s_atom_delete_message = ::XInternAtom(s_display, "WM_DELETE_WINDOW", 0);

    // TODO: Unused
    s_atom_minimized = ::XInternAtom(s_display, "_NET_WM_STATE_HIDDEN", 0);
    s_atom_unminimized = ::XInternAtom(s_display, "_NET_WM_STATE", 0);
    s_atom_maximized_h = ::XInternAtom(s_display, "_NET_WM_STATE_MAXIMIZED_VERT", 0);
    s_atom_maximized_v = ::XInternAtom(s_display, "_NET_WM_STATE_MAXIMIZED_HORZ", 0);

    ::XSetWMProtocols(s_display, s_window, &s_atom_delete_message, 1);

    ::XSelectInput(s_display, s_window, StructureNotifyMask | PropertyChangeMask);
    ::XStoreName(s_display, s_window, title);
    ::XMapWindow(s_display, s_window);

    std::cout << "Opened window" << std::endl;
}

void pr::backend::device::Window::pollEvents()
{
    ::XEvent e;
    ::XNextEvent(s_display, &e);
    if (e.type == ConfigureNotify)
    {
        XConfigureEvent const& xce = e.xconfigure;
        onResizeEvent(xce.width, xce.height, false);
    }
    else if (e.type == ClientMessage)
    {
        if (e.xclient.data.l[0] == long(s_atom_delete_message))
            onCloseEvent();
    }
}
void pr::backend::device::Window::onCloseEvent() { mIsRequestingClose = true; }

void pr::backend::device::Window::onResizeEvent(int w, int h, bool minimized)
{
    if (mWidth == w && mHeight == h && mIsMinimized == minimized)
        return;

    mWidth = w;
    mHeight = h;
    mIsMinimized = minimized;
    mPendingResize = true;
}


#ifdef PR_BACKEND_VULKAN
cc::vector<char const*> pr::backend::device::Window::getRequiredInstanceExtensions()
{
    cc::vector<char const*> res;
    res.reserve(2);
    res.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
    res.push_back(VK_KHR_XLIB_SURFACE_EXTENSION_NAME);
    return res;
}

void pr::backend::device::Window::createVulkanSurface(VkInstance instance, VkSurfaceKHR& out_surface)
{
    VkXlibSurfaceCreateInfoKHR surface_info;
    vk::zero_info_struct(surface_info, VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR);
    surface_info.dpy = s_display;
    surface_info.window = s_window;
    surface_info.pNext = nullptr;
    surface_info.flags = 0;
    PR_VK_VERIFY_SUCCESS(vkCreateXlibSurfaceKHR(instance, &surface_info, nullptr, &out_surface));
}
#endif


#endif
